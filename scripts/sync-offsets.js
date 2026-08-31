const fs = require("fs");
const path = require("path");
const https = require("https");

const BASE = "https://offsets.imtheo.lol";
const ROOT = path.join(__dirname, "..");
const OFFSETS_DIR = path.join(ROOT, "offsets");
const SDK_HEADER = path.join(ROOT, "External", "src", "sdk", "offsets.h");

const FILES = [
  { remote: "Offsets.hpp", local: "offsets.hpp", sdk: true },
  { remote: "Offsets.json", local: "offsets.json", versionArchive: true },
  { remote: "Offsets.cs", local: "offsets.cs" },
  { remote: "Offsets.txt", local: "offsets.txt" },
  { remote: "OffsetsHex.json", local: "offsets-hex.json" },
  { remote: "FFlags.json", local: "fflags.json" },
  { remote: "FFlags.hpp", local: "fflags.hpp" },
  { remote: "FFlags.cs", local: "fflags.cs" },
  { remote: "FFlags.txt", local: "fflags.txt", optional: true },
  { remote: "FFlagsHex.json", local: "fflags-hex.json" },
  { remote: "Struct.hpp", local: "struct.hpp" },
  { remote: "Types.json", local: "types.json" },
];

function fetchBuffer(url, headers) {
  return new Promise((resolve, reject) => {
    https
      .get(url, headers ? { headers } : undefined, (res) => {
        if (res.statusCode >= 300 && res.statusCode < 400 && res.headers.location) {
          fetchBuffer(res.headers.location, headers).then(resolve, reject);
          return;
        }
        if (res.statusCode !== 200) {
          reject(new Error("HTTP " + res.statusCode + " for " + url));
          res.resume();
          return;
        }
        const chunks = [];
        res.on("data", (c) => chunks.push(c));
        res.on("end", () => resolve(Buffer.concat(chunks)));
      })
      .on("error", reject);
  });
}

function parseVersion(jsonText) {
  try {
    const data = JSON.parse(jsonText);
    const v = data["Roblox Version"] || data.RobloxVersion || "";
    return String(v).trim() || null;
  } catch (_) {
    return null;
  }
}

// Upstream dump omits attribute + desync RVAs — merge FRONTIER extensions after sync.
function patchSdkHeader(headerPath) {
  let text = fs.readFileSync(headerPath, "utf8");
  if (!text.includes("namespace Attribute")) {
    text = text.replace(
      /(\s+namespace Instance \{)/,
      [
        "",
        "    namespace Attribute {",
        "         inline constexpr uintptr_t Key = 0x0;",
        "         inline constexpr uintptr_t Size = 0x58;",
        "         inline constexpr uintptr_t Value = 0x18;",
        "         inline constexpr uintptr_t TypeDescriptor = 0x8;",
        "    }",
        "",
        "    namespace AttributesMap {",
        "         inline constexpr uintptr_t Attributes = 0x18;",
        "         inline constexpr uintptr_t Length = 0x0;",
        "    }",
        "",
        "    namespace Instance {",
      ].join("\n")
    );
  }
  if (!text.includes("AttributeContainer")) {
    text = text.replace(
      /(\s+inline constexpr uintptr_t Parent = 0x[0-9a-fA-F]+;\r?\n)/,
      "$1         inline constexpr uintptr_t AttributeContainer = 0x48;\n"
    );
  }
  if (!text.includes("namespace Desync")) {
    text = text.replace(
      /\}\s*$/,
      [
        "",
        "    namespace Desync {",
        "         inline constexpr uintptr_t PhysicsSenderMaxBandwidthBps = 0x7b9fb88;",
        "    }",
        "",
        "}",
      ].join("\n")
    );
  }
  fs.writeFileSync(headerPath, text);
}

async function main() {
  fs.mkdirSync(OFFSETS_DIR, { recursive: true });

  let activeVersion = null;
  let activeArchive = null;
  const syncedAt = new Date().toISOString();
  const routes = [];

  for (const entry of FILES) {
    const url = BASE + "/" + entry.remote;
    process.stdout.write("fetch " + entry.remote + " ... ");
    let buf;
    try {
      buf = await fetchBuffer(url);
    } catch (err) {
      if (entry.optional) {
        console.log("skip (" + err.message + ")");
        continue;
      }
      throw err;
    }
    const outPath = path.join(OFFSETS_DIR, entry.local);
    fs.writeFileSync(outPath, buf);
    console.log(entry.local, "(" + buf.length + " bytes)");

    if (entry.versionArchive) {
      const version = parseVersion(buf.toString("utf8"));
      if (version) {
        activeVersion = version;
        activeArchive = version + ".json";
        const archivePath = path.join(OFFSETS_DIR, activeArchive);
        fs.writeFileSync(archivePath, buf);
        console.log("  archived -> " + activeArchive);
      }
    }

    if (entry.sdk) {
      fs.writeFileSync(SDK_HEADER, buf);
      patchSdkHeader(SDK_HEADER);
      console.log("  sdk -> External/src/sdk/offsets.h");
    }

    routes.push({
      name: entry.remote,
      local: "offsets/" + entry.local,
      upstream: url,
      ahead: "https://trace-host.vercel.app/api/frontier-offsets?file=" + encodeURIComponent(entry.remote),
      github:
        "https://raw.githubusercontent.com/binx-ux/frontier/main/offsets/" + entry.local,
    });
  }

  const manifest = {
    project: "FRONTIER",
    source: BASE,
    weao: "https://weao.xyz/api/versions/current",
    syncedAt,
    activeVersion,
    activeArchive: activeArchive ? "offsets/" + activeArchive : null,
    discord: BASE + "/discord",
    routes,
  };

  fs.writeFileSync(path.join(OFFSETS_DIR, "active.json"), JSON.stringify({ file: activeArchive, version: activeVersion, syncedAt }, null, 2) + "\n");
  fs.writeFileSync(path.join(OFFSETS_DIR, "sources.json"), JSON.stringify(manifest, null, 2) + "\n");

  console.log("\nsynced FRONTIER offsets");
  console.log("  version:", activeVersion || "unknown");
  console.log("  active:", activeArchive || "n/a");

  // Cross-check against WEAO live Roblox Windows channel
  try {
    const weaoBuf = await fetchBuffer("https://weao.xyz/api/versions/current", {
      "User-Agent": "WEAO-3PService",
    });
    const weao = JSON.parse(weaoBuf.toString("utf8"));
    const live = weao && weao.Windows ? String(weao.Windows).trim() : "";
    if (live && activeVersion && live !== activeVersion) {
      console.warn("  WEAO mismatch: live=" + live + " synced=" + activeVersion);
    } else if (live) {
      console.log("  WEAO verified:", live);
    }
  } catch (err) {
    console.log("  WEAO check skipped:", err.message);
  }
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
