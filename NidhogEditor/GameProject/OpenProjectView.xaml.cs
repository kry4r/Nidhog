using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace NidhogEditor.GameProject
{
    /// <summary>
    /// OpenProjectView.xaml 的交互逻辑
    /// </summary>
    public partial class OpenProjectView : UserControl
    {
        public OpenProjectView()
        {
            InitializeComponent();

            Loaded += (s, e) =>
            {
                var item = projectsListBox.ItemContainerGenerator
                .ContainerFromIndex(projectsListBox.SelectedIndex) as ListBoxItem;
                item?.Focus();
            };
        }

        private void OnOpen_Button_Click(object sender, RoutedEventArgs e)
        {
            OpenSelectedProject();
        }

        private void OnListBoxItem_Mouse_DoubleClick(object sender, MouseButtonEventArgs e)
        {
            OpenSelectedProject();
        }
        private void OpenSelectedProject()
        {
            var project = OpenProject.Open(projectsListBox.SelectedItem as ProjectData);
            bool dialogResult = false;
            var win = Window.GetWindow(this);
            if (project != null)
            {
                dialogResult = true;
                win.DataContext = project;
            }
            win.DialogResult = dialogResult;
            win.Close();
        }
    }
}
