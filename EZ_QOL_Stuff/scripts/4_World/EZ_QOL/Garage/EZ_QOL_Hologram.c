// Hologram hooks for EZ_QOL_GarageKit ONLY. Every other item goes straight to super (VGS and vanilla untouched).
// UNVERIFIED signatures (compile check): ProjectionBasedOnParent(), EvaluateCollision(ItemBase action_item = null).
modded class Hologram
{
	override string ProjectionBasedOnParent()
	{
		if (m_Parent && m_Parent.IsInherited(EZ_QOL_GarageKit))
			return "EZ_QOL_GarageHologram";
		return super.ProjectionBasedOnParent();
	}

	override void EvaluateCollision(ItemBase action_item = null)
	{
		if (m_Parent && m_Parent.IsInherited(EZ_QOL_GarageKit))
		{
			vector p, o; string r, d;
			bool ok = EZ_QOL_GaragePlacement.Validate(m_Player, GetProjectionPosition(), GetProjectionOrientation(), p, o, r, d, m_Parent);
			SetIsColliding(!ok);
			return;
		}
		super.EvaluateCollision(action_item);
	}
}
