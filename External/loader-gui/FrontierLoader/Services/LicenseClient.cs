using System;
using System.Threading.Tasks;

namespace FrontierLoader.Services
{
    internal sealed class LicenseResult
    {
        public bool Ok { get; set; }
        public bool Licensed { get; set; }
        public string Token { get; set; } = "";
        public string Message { get; set; } = "";
    }

    // Open-source build: no remote license API / HWID binding.
    internal static class LicenseClient
    {
        public const string OpenToken = "opensource";

        public static string GetHwid() => "opensource";

        public static void LoadSaved(out string token, out string key)
        {
            token = OpenToken;
            key = "";
        }

        public static void Save(string token, string hwid, string key) { }

        public static void Clear() { }

        public static Task<LicenseResult> ActivateKeyAsync(string key, string hwid) =>
            Task.FromResult(new LicenseResult
            {
                Ok = true,
                Licensed = true,
                Token = OpenToken,
                Message = "Open source — no key required."
            });

        public static Task<LicenseResult> ValidateTokenAsync(string token, string hwid) =>
            Task.FromResult(new LicenseResult
            {
                Ok = true,
                Licensed = true,
                Token = string.IsNullOrWhiteSpace(token) ? OpenToken : token,
                Message = "Open source — no key required."
            });
    }
}
