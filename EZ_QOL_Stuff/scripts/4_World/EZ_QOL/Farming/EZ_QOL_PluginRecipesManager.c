// Registers the pineapple slicing recipe. Super first so vanilla and other mods' recipes are untouched.
modded class PluginRecipesManagerBase
{
	override void RegisterRecipies()
	{
		super.RegisterRecipies();
		RegisterRecipe(new EZ_QOL_SlicePineapple);
	}
}
