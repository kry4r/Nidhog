using NidhogEditor.Content;

namespace NidhogEditor.Editors
{
    internal interface IAssetEditor
    {
        Asset Asset { get; }
        void SetAsset(Asset asset);
    }
}