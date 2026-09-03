using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;

namespace FrontierLoader.Services
{
    internal static class LauncherService
    {
        [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
        static extern int GetPrivateProfileInt(string section, string key, int def, string file);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
        static extern bool WritePrivateProfileString(string section, string key, string value, string file);

        static string IniPath => Path.Combine(UpdateClient.LoaderDir, "loader.ini");

        public static void SaveMode(int mode) =>
            WritePrivateProfileString("loader", "mode", mode.ToString(), IniPath);

        public static int LoadMode() =>
            GetPrivateProfileInt("loader", "mode", 0, IniPath);

        public static void Launch(int mode, string token, string hwid)
        {
            if (string.IsNullOrWhiteSpace(token))
                token = LicenseClient.OpenToken;
            if (string.IsNullOrWhiteSpace(hwid))
                hwid = "opensource";

            SaveMode(mode);
            if (mode == 0)
                LaunchUsermode(token, hwid);
            else
                LaunchKernel(token, hwid);
        }

        static void WriteSessionFile(string workDir, string token, string hwid)
        {
            Directory.CreateDirectory(workDir);
            var path = Path.Combine(workDir, "frontier.session");
            File.WriteAllText(path, "token=" + token + "\nhwid=" + (hwid ?? "") + "\n");
        }

        static void LaunchUsermode(string token, string hwid)
        {
            var exe = UpdateClient.UsermodePath;
            if (!File.Exists(exe))
                throw new FileNotFoundException("Missing usermode\\Frontier.exe — run Update first.");

            var work = Path.Combine(UpdateClient.LoaderDir, "usermode");
            WriteSessionFile(work, token, hwid);
            Process.Start(new ProcessStartInfo
            {
                FileName = exe,
                WorkingDirectory = work,
                UseShellExecute = true
            });
        }

        static void LaunchKernel(string token, string hwid)
        {
            var exe = UpdateClient.KernelPath;
            if (!File.Exists(exe))
                throw new FileNotFoundException("Kernel mode is not installed. Run Update or use Usermode.");

            if (!File.Exists(UpdateClient.DriverPath))
                throw new FileNotFoundException("Kernel driver missing. Place kernel\\driver\\FrontierDrv.sys.");

            var work = Path.Combine(UpdateClient.LoaderDir, "kernel");
            WriteSessionFile(work, token, hwid);
            var psi = new ProcessStartInfo
            {
                FileName = exe,
                WorkingDirectory = work,
                Verb = "runas",
                UseShellExecute = true
            };
            Process.Start(psi);
        }
    }
}
