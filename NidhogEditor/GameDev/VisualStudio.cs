using NidhogEditor.Utilities;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
using System.Text;
using System.Threading.Tasks;

namespace NidhogEditor.GameDev
{
    static class VisualStudio
    {
        private static EnvDTE80.DTE2 _vsInstance = null;
        private static readonly string _progID = "VisualStudio.DTE.16.0";

        [DllImport("ole32.dll")]    //binding context
        private static extern int CreateBindCtx(uint reserved, out IBindCtx ppbc);

        [DllImport("ole32.dll")]    //在这个dll中，所以我们需要从这里面导入
        private static extern int GetRunningObjectTable(uint reserved, out IRunningObjectTable pprot);

        public static void OpenVisualStudio(string solutionPath)
        {
            IRunningObjectTable rot = null;
            IEnumMoniker monikerTable = null;
            IBindCtx bindCtx = null;
            try//有可能抛出异常，所以放在try中
            {
                //检查vs是否打开
                if (_vsInstance == null)
                {
                    // 找到并打开vs

                    //返回指向本地运行对象表上的 IRunningObjectTable 接口的指针
                    //获取访问运行对象表的权限
                    var hResult = GetRunningObjectTable(0, out rot);
                    //如果小于0则返回异常
                    if (hResult < 0 || rot == null) throw new COMException($"GetRunningObjectTable() returned HRESULT: {hResult:X8}");

                    //为我们提供一个可枚举的表
                    rot.EnumRunning(out monikerTable);
                    //先对表进行重置
                    monikerTable.Reset();

                    //同样的对其是否可用进行检查
                    hResult = CreateBindCtx(0, out bindCtx);
                    if (hResult < 0 || bindCtx == null) throw new COMException($"CreateBindCtx() returned HRESULT: {hResult:X8}");

                    //遍历表
                    IMoniker[] currentMoniker = new IMoniker[1];
                    while (monikerTable.Next(1, currentMoniker, IntPtr.Zero) == 0)
                    {
                        string name = string.Empty;
                        //获取进程名
                        currentMoniker[0]?.GetDisplayName(bindCtx, null, out name);
                        if (name.Contains(_progID))
                        {
                            hResult = rot.GetObject(currentMoniker[0], out object obj);
                            if (hResult < 0 || obj == null) throw new COMException($"Running object table's GetObject() returned HRESULT: {hResult:X8}");

                            //通过这个转化到instance
                            EnvDTE80.DTE2 dte = obj as EnvDTE80.DTE2;
                            var solutionName = dte.Solution.FullName;
                            if (solutionName == solutionPath)
                            {
                                _vsInstance = dte;
                                break;
                            }
                        }
                    }
                    //从硬盘启用vs
                    if (_vsInstance == null)
                    {
                        Type visualStudioType = Type.GetTypeFromProgID(_progID, true);
                        //创建上面这种Type的instance
                        _vsInstance = Activator.CreateInstance(visualStudioType) as EnvDTE80.DTE2;
                    }
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Logger.Log(MessageType.Error, "failed to open Visual Studio");
            }
            finally
            {
                //资源释放
                if (monikerTable != null) Marshal.ReleaseComObject(monikerTable);
                if (rot != null) Marshal.ReleaseComObject(rot);
                if (bindCtx != null) Marshal.ReleaseComObject(bindCtx);
            }
        }

        public static void CloseVisualStudio()
        {
            if (_vsInstance?.Solution.IsOpen == true)
            {
                //如果是打开的，调用保存命令之后关闭
                _vsInstance.ExecuteCommand("File.SaveAll");
                _vsInstance.Solution.Close(true);
            }
            _vsInstance?.Quit();
        }

        public static bool AddFilesToSolution(string solution, string projectName, string[] files)
        {
            Debug.Assert(files?.Length > 0);
            OpenVisualStudio(solution);
            try
            {
                if (_vsInstance != null)
                {
                    if (!_vsInstance.Solution.IsOpen) _vsInstance.Solution.Open(solution);
                    else _vsInstance.ExecuteCommand("File.SaveAll");

                    foreach (EnvDTE.Project project in _vsInstance.Solution.Projects)
                    {
                        if (project.UniqueName.Contains(projectName))
                        {
                            foreach (var file in files)
                            {
                                project.ProjectItems.AddFromFile(file);
                            }
                        }
                    }

                    var cpp = files.FirstOrDefault(x => Path.GetExtension(x) == ".cpp");
                    if (!string.IsNullOrEmpty(cpp))
                    {
                        _vsInstance.ItemOperations.OpenFile(cpp, EnvDTE.Constants.vsViewKindTextView).Visible = true;
                    }
                    _vsInstance.MainWindow.Activate();
                    _vsInstance.MainWindow.Visible = true;
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Debug.WriteLine("failed to add files to Visual Studio project");
                return false;
            }
            return true;
        }
    }
}

