// EZ_QOL pineapple (moved from EZLoderScreen 10.1, was EZ_SlicePineapple): cut a whole EZ_Pineapple into 4 EZ_PineappleSlice, modelled on vanilla SlicePumpkin.
class EZ_QOL_SlicePineapple extends RecipeBase
{
	override void Init()
	{
		m_Name = "Slice pineapple";
		m_IsInstaRecipe = false;
		m_AnimationLength = 1;
		m_Specialty = 0.02;

		// conditions
		m_MinDamageIngredient[0] = -1;
		m_MaxDamageIngredient[0] = 3;
		m_MinQuantityIngredient[0] = -1;
		m_MaxQuantityIngredient[0] = -1;

		m_MinDamageIngredient[1] = -1;
		m_MaxDamageIngredient[1] = 3;
		m_MinQuantityIngredient[1] = -1;
		m_MaxQuantityIngredient[1] = -1;

		// ingredient 1: the whole pineapple
		InsertIngredient(0, "EZ_Pineapple");
		m_IngredientAddHealth[0] = 0;
		m_IngredientSetHealth[0] = -1;
		m_IngredientAddQuantity[0] = 0;
		m_IngredientDestroy[0] = true;
		m_IngredientUseSoftSkills[0] = false;

		// ingredient 2: sharp tools (same list as vanilla SlicePumpkin)
		InsertIngredient(1, "Sickle");
		InsertIngredient(1, "KukriKnife");
		InsertIngredient(1, "FangeKnife");
		InsertIngredient(1, "Hacksaw");
		InsertIngredient(1, "HandSaw");
		InsertIngredient(1, "KitchenKnife");
		InsertIngredient(1, "SteakKnife");
		InsertIngredient(1, "HayHook");
		InsertIngredient(1, "StoneKnife");
		InsertIngredient(1, "Cleaver");
		InsertIngredient(1, "CombatKnife");
		InsertIngredient(1, "HuntingKnife");
		InsertIngredient(1, "Machete");
		InsertIngredient(1, "CrudeMachete");
		InsertIngredient(1, "OrientalMachete");
		InsertIngredient(1, "WoodAxe");
		InsertIngredient(1, "Hatchet");
		InsertIngredient(1, "FirefighterAxe");
		InsertIngredient(1, "Sword");
		InsertIngredient(1, "AK_Bayonet");
		InsertIngredient(1, "M9A1_Bayonet");
		InsertIngredient(1, "Mosin_Bayonet");
		InsertIngredient(1, "SKS_Bayonet");
		InsertIngredient(1, "BoneKnife");
		m_IngredientAddHealth[1] = -3;
		m_IngredientSetHealth[1] = -1;
		m_IngredientAddQuantity[1] = 0;
		m_IngredientDestroy[1] = false;
		m_IngredientUseSoftSkills[1] = true;

		// results: 4 slices, health inherited from the pineapple (ingredient 0)

		AddResult("EZ_PineappleSlice");
		m_ResultSetFullQuantity[0] = true;
		m_ResultSetQuantity[0] = -1;
		m_ResultSetHealth[0] = -1;
		m_ResultInheritsHealth[0] = 0;
		m_ResultInheritsColor[0] = -1;
		m_ResultToInventory[0] = -2;
		m_ResultUseSoftSkills[0] = false;
		m_ResultReplacesIngredient[0] = -1;

		AddResult("EZ_PineappleSlice");
		m_ResultSetFullQuantity[1] = true;
		m_ResultSetQuantity[1] = -1;
		m_ResultSetHealth[1] = -1;
		m_ResultInheritsHealth[1] = 0;
		m_ResultInheritsColor[1] = -1;
		m_ResultToInventory[1] = -2;
		m_ResultUseSoftSkills[1] = false;
		m_ResultReplacesIngredient[1] = -1;

		AddResult("EZ_PineappleSlice");
		m_ResultSetFullQuantity[2] = true;
		m_ResultSetQuantity[2] = -1;
		m_ResultSetHealth[2] = -1;
		m_ResultInheritsHealth[2] = 0;
		m_ResultInheritsColor[2] = -1;
		m_ResultToInventory[2] = -2;
		m_ResultUseSoftSkills[2] = false;
		m_ResultReplacesIngredient[2] = -1;

		AddResult("EZ_PineappleSlice");
		m_ResultSetFullQuantity[3] = true;
		m_ResultSetQuantity[3] = -1;
		m_ResultSetHealth[3] = -1;
		m_ResultInheritsHealth[3] = 0;
		m_ResultInheritsColor[3] = -1;
		m_ResultToInventory[3] = -2;
		m_ResultUseSoftSkills[3] = false;
		m_ResultReplacesIngredient[3] = -1;
	}

	override bool CanDo(ItemBase ingredients[], PlayerBase player)
	{
		return true;
	}

	override void Do(ItemBase ingredients[], PlayerBase player, array<ItemBase> results, float specialty_weight)
	{
		ItemBase pineapple = ingredients[0];
		if (!pineapple)
			return;

		// split the remaining quantity of the pineapple across the slices
		float total = pineapple.GetQuantity();
		float perSlice = total / results.Count();
		foreach (ItemBase slice : results)
		{
			if (!slice)
				continue;
			if (perSlice > 0)
				slice.SetQuantity(Math.Min(perSlice, slice.GetQuantityMax()));
			// keep the food stage (raw/rotten/...) of the fruit
			Edible_Base src = Edible_Base.Cast(pineapple);
			Edible_Base dst = Edible_Base.Cast(slice);
			if (src && dst && src.HasFoodStage() && dst.HasFoodStage())
				dst.ChangeFoodStage(src.GetFoodStageType());
		}
		// Sticky hands stays in EZLoderScreen; EZ_QOL deliberately has no call into it (no cross-mod dependency).
	}
}
