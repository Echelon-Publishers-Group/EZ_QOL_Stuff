// Garage + tablet + vehicle flip + sleeping bag action registration. Super first, so vanilla/Expansion/VGS/drone registrations are untouched.
modded class ActionConstructor
{
	override void RegisterActions(TTypenameArray actions)
	{
		super.RegisterActions(actions);
		actions.Insert(EZ_QOL_ActionDeployGarageKit);
		actions.Insert(EZ_QOL_ActionPackGarage);
		actions.Insert(EZ_QOL_ActionParkVehicle);
		actions.Insert(EZ_QOL_ActionRetrieveVehicle);
		actions.Insert(EZ_QOL_ActionOpenTablet);
		actions.Insert(EZ_QOL_ActionFlipVehicle);
		actions.Insert(EZ_QOL_ActionDeploySleepingBag);
		actions.Insert(EZ_QOL_ActionPackSleepingBag);
		actions.Insert(EZ_QOL_ActionClaimSleepingBag);
		actions.Insert(EZ_QOL_ActionRepairSleepingBag);
	}
}
