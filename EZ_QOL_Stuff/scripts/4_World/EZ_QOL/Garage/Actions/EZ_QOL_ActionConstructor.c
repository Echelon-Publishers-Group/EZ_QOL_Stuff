// Garage + tablet action registration. Super first, so vanilla/Expansion/VGS/drone registrations are untouched.
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
	}
}
