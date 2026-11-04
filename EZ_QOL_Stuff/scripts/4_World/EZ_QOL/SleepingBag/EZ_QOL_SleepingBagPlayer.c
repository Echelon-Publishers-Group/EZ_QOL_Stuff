// Client side of EZ_QOL_RPC_BAG_NOTIFY: show the server's sleeping bag message.
modded class PlayerBase
{
	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		super.OnRPC(sender, rpc_type, ctx);
		if (rpc_type != EZ_QOL_RPC_BAG_NOTIFY || GetGame().IsDedicatedServer())
			return;
		Param1<string> p = new Param1<string>("");
		if (!ctx.Read(p))
			return;
		NotificationSystem.AddNotificationExtended(6, "#STR_EZ_QOL_BAG_NAME", p.param1, "set:dayz_gui image:icon_tent"); // icon name [UNVERIFIED]
	}
}
