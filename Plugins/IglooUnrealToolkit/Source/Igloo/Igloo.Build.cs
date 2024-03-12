// Some copyright should be here...

using UnrealBuildTool;
using System.IO;


public class Igloo : ModuleRules
{

    private string ModulePath
    {
        get { return ModuleDirectory; }
    }

//    private string ThirdPartyPath
//    {
//        get { return Path.GetFullPath(Path.Combine(ModulePath, "../../ThirdParty/")); }
//    }
    
	public Igloo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		/*
		PublicIncludePaths.AddRange(
            new string[] {
				"Igloo/Public",			
				// ... add public include paths required here ...
			}
        );
		*/
        

        PrivateIncludePaths.AddRange(
            new string[] {
				"Igloo/Private",
				Path.Combine(Path.GetFullPath(Target.RelativeEnginePath), @"Source/Runtime/Renderer/Private"),
				// ... add other private include paths required here ...
			}
		);
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Engine",
				// ... add other public dependencies that you statically link with here ...
			}
		);

		PrivateDependencyModuleNames.AddRange(new string[]
			{
				"CoreUObject",
				"Slate",
				"SlateCore",
				"RHI",
				"Renderer",
				"RenderCore",
			//	"ShaderCore",
			}
		);



		//	PublicDependencyModuleNames.Add("Core");
		//	PublicDependencyModuleNames.Add("Engine");
		//	PublicDependencyModuleNames.Add("MaterialShaderQualitySettings");
		//	PublicDependencyModuleNames.Add("Renderer");

		//	PublicIncludePathModuleNames.AddRange(new string[] { "Renderer" });

		//DynamicallyLoadedModuleNames.AddRange(new string[] { "Renderer" });

		/*
				DynamicallyLoadedModuleNames.AddRange(
					new string[]
					{

						// ... add any modules that your module loads dynamically here ...
					}
				);

				if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
				{
				//    string PlatformString = (Target.Platform == UnrealTargetPlatform.Win64) ? "amd64" : "x86";
				//    PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "Spout/lib", PlatformString, "Spout.lib"));

				//    RuntimeDependencies.Add(new RuntimeDependency(Path.Combine(ThirdPartyPath, "Spout/lib", PlatformString, "Spout.dll")));

				}
		*/

	}
}
