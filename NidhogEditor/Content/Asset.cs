using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NidhogEditor.Content
{
    //资产类型
    enum AssetType
    {
        Unknown,
        Animation,
        Audio,
        Material,
        Mesh,
        Skeleton,
        Texture,
    }
    
    abstract class Asset : ViewModelBase
    {
        public AssetType Type { get; private set; }
    
        public Asset(AssetType type)
        {
            Debug.Assert(type != AssetType.Unknown);
            Type = type;
            }
        }
    
}
