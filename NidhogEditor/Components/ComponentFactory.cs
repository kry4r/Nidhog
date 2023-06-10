using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;


//类比于一个struct，
namespace NidhogEditor.Components
{
    //包括所以Component种类，可用在这添加Component种类
    enum ComponentType
    {
        Transform,
        Script,
    }

    //就只有一个函数，其他data告诉该函数如何创建一个Component
    static class ComponentFactory
    {
        private static readonly Func<GameEntity, object, Component>[] _function =
            new Func<GameEntity, object, Component>[]
            {
                //注意这里面的顺序需要与enum相同
                (entity, data) => new Transform(entity),
                (entity, data) => new Script(entity){ Name = (string)data},
            };

        public static Func<GameEntity, object, Component> GetCreationFunction(ComponentType componentType)
        {
            Debug.Assert((int)componentType < _function.Length);
            return _function[(int)componentType];
        }
    }
}
