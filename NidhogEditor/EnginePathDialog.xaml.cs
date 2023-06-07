using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;


namespace NidhogEditor
{
    /// <summary>
    /// EnginePathDialog.xaml 的交互逻辑
    /// </summary>
    public partial class EnginePathDialog : Window
    {
        public string NidhogPath { get; private set; }
        public EnginePathDialog()
        {
            InitializeComponent();
            Owner = Application.Current.MainWindow;
        }

        private void OnOk_Button_Click(object sender, RoutedEventArgs e)
        {
            var path = pathTextBox.Text.Trim();
            messageTextBlock.Text = string.Empty;
            if (string.IsNullOrEmpty(path))
            {
                messageTextBlock.Text = "路径无效";
            }
            else if (path.IndexOfAny(Path.GetInvalidPathChars()) != -1)
            {
                messageTextBlock.Text = "路径中使用了无效字符";
            }
            else if (!Directory.Exists(Path.Combine(path, @"Engine\EngineAPI\")))
            {
                messageTextBlock.Text = "无法在指定位置找到引擎";
            }
            if (string.IsNullOrEmpty(messageTextBlock.Text))
            {
                if (!path.EndsWith(@"\")) path += @"\\";
                NidhogPath = path;
                DialogResult = true;
                Close();
            }
        }
    }
}
