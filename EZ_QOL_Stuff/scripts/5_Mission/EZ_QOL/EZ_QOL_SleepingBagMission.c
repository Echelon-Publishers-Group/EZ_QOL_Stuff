// Sleeping bag respawn through vanilla MissionServer hooks only.
// Fresh spawn: OnClientNewEvent gets the bag position before super creates the character.
// Alive login: OnClientReadyEvent moves a living character to the bag (EnableAliveLoginSpawn).
modded class MissionServer
{
	override void OnInit()
	{
		super.OnInit();
		EZ_QOL_SleepingBagConfig.Load();
		EZ_QOL_SleepingBagRegistry.Get();
	}

	override PlayerBase OnClientNewEvent(PlayerIdentity identity, vector pos, ParamsReadContext ctx)
	{
		EZ_SleepingBag_Deployed bag;
		bool atBag = false;
		EZ_QOL_SleepingBagConfig cfg = EZ_QOL_SleepingBagConfig.Get();
		if (identity && cfg.Enabled && cfg.EnableFreshSpawn && !EZ_QOL_SleepingBagRegistry.s_ExternalSpawnSelect)
		{
			vector bagPos;
			string reason = EZ_QOL_SleepingBagRegistry.Get().GetSpawn(identity.GetId(), bagPos, bag);
			if (reason == "")
			{
				pos = bagPos;
				atBag = true;
			}
			else if (reason != "no bag" && reason != "disabled")
			{
				EZ_QOL_Log.Info("SleepingBag fresh spawn skipped uid=" + identity.GetId() + ": " + reason);
				EZ_QOL_DelayedBagNotify(identity, reason);
			}
		}
		PlayerBase pb = super.OnClientNewEvent(identity, pos, ctx);
		if (atBag && pb)
		{
			EZ_QOL_SleepingBagRegistry.Get().OnSpawned(identity.GetId(), bag);
			if (cfg.SpawnHealthPercent < 100)
				pb.SetHealth01("", "Health", cfg.SpawnHealthPercent / 100.0);
			EZ_QOL_DelayedBagNotify(identity, "You woke up at your sleeping bag");
		}
		return pb;
	}

	override void OnClientReadyEvent(PlayerIdentity identity, PlayerBase player)
	{
		super.OnClientReadyEvent(identity, player);
		EZ_QOL_SleepingBagConfig cfg = EZ_QOL_SleepingBagConfig.Get();
		if (!identity || !player || !player.IsAlive() || !cfg.Enabled || !cfg.EnableAliveLoginSpawn)
			return;
		if (EZ_QOL_SleepingBagRegistry.s_ExternalSpawnSelect)
			return;
		EZ_QOL_SleepingBagRegistry reg = EZ_QOL_SleepingBagRegistry.Get();
		EZ_QOL_SleepingBagRecord r = reg.GetRecord(identity.GetId());
		if (!r || r.BagKey == "")
			return;
		if (vector.Distance(player.GetPosition(), r.Pos) <= cfg.AlreadyAtBagRadius)
			return;
		vector bagPos;
		EZ_SleepingBag_Deployed bag;
		string reason = reg.GetSpawn(identity.GetId(), bagPos, bag);
		if (reason != "")
			return; // silent: a normal login should not be spammed with cooldown text
		player.SetPosition(bagPos);
		reg.OnSpawned(identity.GetId(), bag);
		EZ_QOL_DelayedBagNotify(identity, "You woke up at your sleeping bag");
	}

	protected void EZ_QOL_DelayedBagNotify(PlayerIdentity identity, string msg)
	{
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(EZ_QOL_SendBagNotify, 8000, false, identity, msg);
	}

	protected void EZ_QOL_SendBagNotify(PlayerIdentity identity, string msg)
	{
		if (!identity)
			return;
		PlayerBase pb = PlayerBase.Cast(identity.GetPlayer());
		EZ_QOL_SleepingBagRegistry.Notify(pb, msg);
	}
}
