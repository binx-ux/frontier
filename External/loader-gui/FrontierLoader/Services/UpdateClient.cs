using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Net.Http;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;

namespace FrontierLoader.Services
{
    internal sealed class Manifest
    {
        public int Version { get; set; }
        public string Display { get; set; } = "";
        public string UsermodeUrl { get; set; } = "";
        public string KernelUrl { get; set; } = "";
        public string DriverUrl { get; set; } = "";
        public bool KernelAvailable { get; set; }
    }

    internal static class UpdateClient
    {
        static readonly HttpClient Http = new HttpClient { Timeout = TimeSpan.FromMinutes(10) };

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
        static extern int GetPrivateProfileInt(string section, string key, int def, string file);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
        static extern int GetPrivateProfileString(string section, string key, string def, System.Text.StringBuilder ret, int size, string file);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
        static extern bool WritePrivateProfileString(string section, string key, string value, string file);

        static string IniPath => Path.Combine(LoaderDir, "loader.ini");

        public static string LoaderDir => AppDomain.CurrentDomain.BaseDirectory.TrimEnd('\\');

        public static string UsermodePath =>
            Path.Combine(LoaderDir, "usermode", FrontierConfig.UsermodeExe);

        public static string KernelPath =>
            Path.Combine(LoaderDir, "kernel", FrontierConfig.KernelExe);

        public static string DriverPath =>
            Path.Combine(LoaderDir, "kernel", "driver", FrontierConfig.DriverSys);

        public static int LoadLocalVersion() =>
            GetPrivateProfileInt("loader", "localVersion", 0, IniPath);

        public static string LoadInstalledDisplay()
        {
            var sb = new System.Text.StringBuilder(128);
            GetPrivateProfileString("loader", "installedDisplay", "", sb, sb.Capacity, IniPath);
            return sb.ToString();
        }

        public static void SaveInstallRecord(int version, string display)
        {
            WritePrivateProfileString("loader", "localVersion", version.ToString(), IniPath);
            WritePrivateProfileString("loader", "installedDisplay", display, IniPath);
        }

        public static async Task<Manifest?> FetchManifestAsync()
        {
            var json = await Http.GetStringAsync(FrontierConfig.ManifestUrl).ConfigureAwait(false);
            using var doc = JsonDocument.Parse(json);
            var r = doc.RootElement;
            return new Manifest
            {
                Version = r.TryGetProperty("version", out var v) ? v.GetInt32() : 0,
                Display = r.TryGetProperty("display", out var d) ? d.GetString() ?? "" : "",
                UsermodeUrl = r.TryGetProperty("usermode_url", out var u) ? u.GetString() ?? "" : "",
                KernelUrl = r.TryGetProperty("kernel_url", out var k) ? k.GetString() ?? "" : "",
                DriverUrl = r.TryGetProperty("driver_url", out var dr) ? dr.GetString() ?? "" : "",
                KernelAvailable = r.TryGetProperty("kernel_available", out var ka) && ka.GetBoolean()
            };
        }

        public static bool NeedsUpdate(Manifest m)
        {
            if (!File.Exists(UsermodePath)) return true;
            var local = LoadLocalVersion();
            var display = LoadInstalledDisplay();
            if (local > 0 && local >= m.Version && string.Equals(display, m.Display, StringComparison.OrdinalIgnoreCase))
                return false;
            if (local < m.Version) return true;
            return !string.Equals(display, m.Display, StringComparison.OrdinalIgnoreCase);
        }

        public static bool KernelReady() =>
            File.Exists(KernelPath) && File.Exists(DriverPath);

        public static async Task EnsureUpdatedAsync(Manifest m, int launchMode,
            IProgress<(float pct, string msg)>? progress, CancellationToken ct)
        {
            Directory.CreateDirectory(Path.Combine(LoaderDir, "usermode"));
            if (NeedsUpdate(m) && !string.IsNullOrEmpty(m.UsermodeUrl))
            {
                progress?.Report((0.1f, "Downloading usermode..."));
                await DownloadFileAsync(m.UsermodeUrl, UsermodePath + ".new", progress, 0.1f, 0.55f, ct).ConfigureAwait(false);
                AtomicReplace(UsermodePath + ".new", UsermodePath);
                SaveInstallRecord(m.Version, m.Display);
            }

            if (launchMode == 1 && m.KernelAvailable)
            {
                if (!KernelReady())
                {
                    if (string.IsNullOrEmpty(m.KernelUrl) || string.IsNullOrEmpty(m.DriverUrl))
                        throw new InvalidOperationException("Kernel bundle not available in manifest.");

                    Directory.CreateDirectory(Path.Combine(LoaderDir, "kernel"));
                    Directory.CreateDirectory(Path.Combine(LoaderDir, "kernel", "driver"));

                    progress?.Report((0.6f, "Downloading kernel..."));
                    await DownloadFileAsync(m.KernelUrl, KernelPath + ".new", progress, 0.6f, 0.8f, ct).ConfigureAwait(false);
                    AtomicReplace(KernelPath + ".new", KernelPath);

                    progress?.Report((0.85f, "Downloading driver..."));
                    await DownloadFileAsync(m.DriverUrl, DriverPath + ".new", progress, 0.85f, 0.98f, ct).ConfigureAwait(false);
                    AtomicReplace(DriverPath + ".new", DriverPath);
                }
            }

            progress?.Report((1f, "Ready"));
        }

        static async Task DownloadFileAsync(string url, string dest, IProgress<(float, string)>? progress,
            float start, float end, CancellationToken ct)
        {
            using var resp = await Http.GetAsync(url, HttpCompletionOption.ResponseHeadersRead, ct).ConfigureAwait(false);
            resp.EnsureSuccessStatusCode();
            var total = resp.Content.Headers.ContentLength ?? -1;
            using var stream = await resp.Content.ReadAsStreamAsync().ConfigureAwait(false);
            using var file = File.Create(dest);
            var buffer = new byte[81920];
            long read = 0;
            int n;
            while ((n = await stream.ReadAsync(buffer, 0, buffer.Length, ct).ConfigureAwait(false)) > 0)
            {
                await file.WriteAsync(buffer, 0, n, ct).ConfigureAwait(false);
                read += n;
                if (total > 0)
                {
                    var p = start + (end - start) * (read / (float)total);
                    progress?.Report((p, "Downloading..."));
                }
            }
        }

        static void AtomicReplace(string temp, string final)
        {
            if (File.Exists(final))
                File.Delete(final);
            File.Move(temp, final);
        }
    }
}
