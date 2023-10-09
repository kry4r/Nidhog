﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;
using System.IO;
using System.Security.Cryptography;
using System.Diagnostics;
using NidhogEditor.Content;
using System.ComponentModel;
using System.Windows.Media.Imaging;
using System.Windows.Media.TextFormatting;

namespace NidhogEditor
{
    static class EnumExtensions
    {
        public static string GetDescription(this Enum value)
        {
            return (value.GetType().GetField(value.ToString()).GetCustomAttributes(typeof(DescriptionAttribute), false) as DescriptionAttribute[]).FirstOrDefault()?.Description ?? value.ToString();
        }
    }

    static class VisualExtensions
    {
        //调用拓展方法在visual tree中找到visualparent
        public static T FindVisualParent<T>(this DependencyObject depObject) where T : DependencyObject
        {
            if (!(depObject is Visual)) return null;

            var parent = VisualTreeHelper.GetParent(depObject);
            while (parent != null)
            {
                if (parent is T type)
                {
                    return type;
                }
                parent = VisualTreeHelper.GetParent(parent);
            }
            return null;
        }
    }

    public static class ContentHelper
    {
        public static string[] MeshFileExtensions = { ".fbx" };
        public static string[] ImageFileExtensions = { ".bmp", ",png", ".jpg", ".jpeg", ".tiff", ".tif", ".tga", ".dds", ".hdr" };
        public static string[] AudioFileExtensions = { ".ogg", ".wav" };
        public static string GetRandomString(int length = 8)
        {
            if (length <= 0) length = 8;
            var n = length / 11;
            var sb = new StringBuilder();
            for (int i = 0; i <= n; ++i)
            {
                sb.Append(Path.GetRandomFileName().Replace(".", ""));
            }

            return sb.ToString(0, length);
        }

        public static bool IsDirectory(string path)
        {
            try
            {
                return File.GetAttributes(path).HasFlag(FileAttributes.Directory);
            }
            catch (Exception ex) { Debug.WriteLine(ex.Message); }
            return false;
        }

        public static bool IsDirectory(this FileInfo info) => info.Attributes.HasFlag(FileAttributes.Directory);

        public static bool IsOlder(this DateTime date, DateTime other) => date < other;


        public static string SanitizeFileName(string name)
        {
            Debug.Assert(!string.IsNullOrEmpty(name));
            var path = new StringBuilder(name.Substring(0, name.LastIndexOf(Path.DirectorySeparatorChar) + 1));
            var file = new StringBuilder(name[(name.LastIndexOf(Path.DirectorySeparatorChar) + 1)..]);
            foreach (var c in Path.GetInvalidPathChars())
            {
                path.Replace(c, '_');
            }
            foreach (var c in Path.GetInvalidFileNameChars())
            {
                file.Replace(c, '_');
            }
            return path.Append(file).ToString();
        }

        public static byte[] ComputeHash(byte[] data, int offset = 0, int count = 0)
        {
            if (data?.Length > 0)
            {
                using var sha256 = SHA256.Create();
                return sha256.ComputeHash(data, offset, count > 0 ? count : data.Length);
            }
            return null;
        }

        public static async Task ImportFilesAsync(string[] files, string destination)
        {
            try
            {
                Debug.Assert(!string.IsNullOrEmpty(destination));
                ContentWatcher.EnableFileWatcher(false);
                var tasks = files.Select(async file => await Task.Run(() => { Import(file, destination); }));
                await Task.WhenAll(tasks);
            }
            catch (Exception ex)
            {
                Debug.WriteLine($"Failed to import files to {destination}");
                Debug.WriteLine(ex.Message);
            }
            finally
            {
                ContentWatcher.EnableFileWatcher(true);
            }
        }

        private static Asset Import(string file, string destination)
        {
            Debug.Assert(!string.IsNullOrEmpty(file));
            if (IsDirectory(file)) return null;
            var name = Path.GetFileNameWithoutExtension(file).ToLower();
            var ext = Path.GetExtension(file).ToLower();

            Asset asset = ext switch
            {
                { } when MeshFileExtensions.Contains(ext) => new Content.Geometry(),
                { } when ImageFileExtensions.Contains(ext) => new Texture(),
                { } when AudioFileExtensions.Contains(ext) => null,
                _ => null
            };

            if (asset != null)
            {
                Import(asset, name, file, destination);
            }
            return asset;
        }

        private static void Import(Asset asset, string name, string file, string destination)
        {
            destination = destination?.Trim();
            Debug.Assert(asset != null);
            Debug.Assert(!string.IsNullOrEmpty(destination) && Directory.Exists(destination));

            if (!destination.EndsWith(Path.DirectorySeparatorChar)) destination += Path.DirectorySeparatorChar;
            asset.FullPath = destination + name + Asset.AssetFileExtension;
            bool importSucceeded = false;
            try
            {
                // NOTE: FullPath must be set before we call asset.Import().
                importSucceeded = !string.IsNullOrEmpty(file) && asset.Import(file);

                if (importSucceeded)
                {
                    asset.Save(asset.FullPath);
                }

                return;
            }
            finally
            {
                // TODO: UI stuff for import status
            }
        }
    }

    static class BitmapHelper
    {
        public static byte[] CreateThumbnail(BitmapSource image, int maxWidth, int maxHeight)
        {
            var scaleX = maxWidth / (double)image.PixelWidth;
            var scaleY = maxHeight / (double)image.PixelHeight;
            var ratio = Math.Min(scaleX, scaleY);

            var thumbnail = new TransformedBitmap(image, new ScaleTransform(ratio, ratio, 0.5, 0.5));

            using var memStream = new MemoryStream();
            memStream.SetLength(0);

            var encoder = new PngBitmapEncoder();
            encoder.Frames.Add(BitmapFrame.Create(thumbnail));
            encoder.Save(memStream);

            return memStream.ToArray();
        }

        internal static BitmapSource ImageFromSlice(Slice slice, bool isNormalMap = false)
        {
            var data = slice.RawContent;
            var bytesPerPixel = data.Length / (slice.Width * slice.Height);
            var stride = slice.Width * bytesPerPixel; // This should be the same as slice.RowPitch
            var format = PixelFormats.Default;
            byte[] bgrData = null;

            if (bytesPerPixel == 16) format = PixelFormats.Rgba128Float;
            else if (bytesPerPixel == 4) format = PixelFormats.Bgra32;
            else if (bytesPerPixel == 3 || bytesPerPixel == 2) format = PixelFormats.Bgr24;
            else if (bytesPerPixel == 1) format = PixelFormats.Gray8;

            if (bytesPerPixel == 16)
            {
                bgrData = new byte[data.Length];
                Buffer.BlockCopy(data, 0, bgrData, 0, data.Length);
            }
            else if (bytesPerPixel == 4 || bytesPerPixel == 3)
            {
                bgrData = new byte[data.Length];
                Buffer.BlockCopy(data, 0, bgrData, 0, data.Length);

                // swap R and B channels: RGB -> BGR
                for (int i = 0; i < data.Length; i += bytesPerPixel)
                {
                    var r = bgrData[i + 2];
                    bgrData[i + 2] = bgrData[i];
                    bgrData[i] = r;
                }
            }
            else if (bytesPerPixel == 2)
            {
                bgrData = new byte[slice.Width * slice.Height * 3];
                stride = slice.Width * 3;
                var inv255 = 1.0 / 255.0;
                var isNM = isNormalMap ? 1 : 0;
                int index = 0;
                for (int i = 0; i < data.Length; i += 2)
                {
                    var r = data[i + 0] * inv255 * 2.0 - 1.0;
                    var g = data[i + 1] * inv255 * 2.0 - 1.0;
                    var b = (Math.Sqrt(Math.Clamp(1.0 - (r * r + g * g), 0.0, 1.0)) + 1.0) * 0.5 * 255.0;
                    bgrData[index + 2] = data[i + 0];
                    bgrData[index + 1] = data[i + 1];
                    bgrData[index + 0] = (byte)(b * isNM);
                    index += 3;
                }
            }
            else if (bytesPerPixel == 1)
            {
                bgrData = new byte[data.Length];
                Buffer.BlockCopy(data, 0, bgrData, 0, data.Length);
            }

            BitmapSource image = null;
            if (bgrData != null)
            {
                image = BitmapSource.Create(slice.Width, slice.Height, 96.0, 96.0, format, null, bgrData, stride);
            }
            return image;
        }
    }
}

