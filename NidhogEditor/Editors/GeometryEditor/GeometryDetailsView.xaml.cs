﻿using System;
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

namespace NidhogEditor.Editors
{
    /// <summary>
    /// GeometryDetailsView.xaml 的交互逻辑
    /// </summary>
    public partial class GeometryDetailsView : UserControl
    {
        public GeometryDetailsView()
        {
            InitializeComponent();
        }

        private void OnHighlight_CheckBox_Click(object sender, RoutedEventArgs e)
        {
            var vm = DataContext as GeometryEditor;
            foreach (var m in vm.MeshRenderer.Meshes)
            {
                m.IsHighlighted = false;
            }

            var checkBox = sender as CheckBox;
            (checkBox.DataContext as MeshRendererVertexData).IsHighlighted = checkBox.IsChecked == true;
        }

        private void OnIsolate_CheckBox_Click(object sender, RoutedEventArgs e)
        {
            var vm = DataContext as GeometryEditor;
            foreach (var m in vm.MeshRenderer.Meshes)
            {
                m.IsIsolated = false;
            }

            var checkBox = sender as CheckBox;
            var mesh = checkBox.DataContext as MeshRendererVertexData;
            mesh.IsIsolated = checkBox.IsChecked == true;

            if (Tag is GeometryView geometryView)
            {
                geometryView.SetGeometry(mesh.IsIsolated ? vm.MeshRenderer.Meshes.IndexOf(mesh) : -1);
            }

        }
    }
}
