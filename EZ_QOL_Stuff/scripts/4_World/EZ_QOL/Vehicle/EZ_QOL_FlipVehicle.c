// EZ_QOL vehicle flip (moved from EZLoderScreen 10.1 EZ_FlipVehicle.c, classes renamed EZ_QOL_*).
// Hold interact next to an overturned, empty, engine-off vehicle, or press the EZ_QOL flip key (default F6).
// Server-authoritative: conditions re-checked on server; only the server moves the vehicle.
// Works for anything derived from CarScript (vanilla, CarPack, Expansion cars built on CarScript).
class EZ_QOL_Flip
{
	static const float TIME = 6.0;        // seconds of interaction
	static const float MAX_UP_Y = 0.6;    // vehicle "up" vector Y below this = overturned (~53 deg+)
	static const float MAX_SPEED = 1.0;   // m/s
	static const float LIFT = 1.0;        // metres lifted before dropping on wheels

	static CarScript GetCar(ActionTarget target)
	{
		if (!target)
			return null;
		CarScript car = CarScript.Cast(target.GetObject());
		if (!car)
			car = CarScript.Cast(target.GetParent());
		return car;
	}

	static bool CanFlip(PlayerBase player, CarScript car)
	{
		if (!player || !car || player.IsInVehicle())
			return false;
		if (car.IsDamageDestroyed())
			return false;
		if (car.EngineIsOn())
			return false;
		for (int i = 0; i < car.CrewSize(); i++)
		{
			if (car.CrewMember(i))
				return false;
		}
		if (GetVelocity(car).Length() > MAX_SPEED)
			return false;
		vector up = car.GetDirectionUp();
		return up[1] < MAX_UP_Y;
	}

	static void DoFlip(CarScript car)
	{
		vector pos = car.GetPosition();
		vector ori = car.GetOrientation(); // yaw, pitch, roll
		float surfY = GetGame().SurfaceY(pos[0], pos[2]);
		float newY = Math.Max(pos[1], surfY) + LIFT;
		pos[1] = newY;
		car.SetPosition(pos);
		car.SetOrientation(Vector(ori[0], 0, 0));
		SetVelocity(car, vector.Zero);
		dBodySetAngularVelocity(car, vector.Zero);
		dBodyActive(car, ActiveState.ACTIVE);
		car.SetSynchDirty();
	}
}

// Keybind flip: server re-validates, flips after KEY_DELAY if still valid.
modded class PlayerBase
{
	float m_EZ_QOL_LastFlipReq;
	static const int EZ_QOL_FLIP_KEY_DELAY_MS = 4000;
	static const float EZ_QOL_FLIP_COOLDOWN = 10;

	void EZ_QOL_FlipKeyFinish(CarScript car)
	{
		if (!car || !IsAlive() || vector.Distance(car.GetPosition(), GetPosition()) > 6)
			return;
		if (!EZ_QOL_Flip.CanFlip(this, car))
		{
			NotificationSystem.SendNotificationToPlayerExtended(this, 4, "Flip vehicle", "Flip cancelled (vehicle occupied, running or moving).", "set:dayz_gui image:icon_info");
			return;
		}
		EZ_QOL_Flip.DoFlip(car);
	}

	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		super.OnRPC(sender, rpc_type, ctx);
		if (rpc_type != EZ_QOL_RPC_FLIP_REQUEST)
			return;
		if (!GetGame().IsServer() || !sender || !GetIdentity() || sender.GetPlainId() != GetIdentity().GetPlainId())
			return;
		Param1<Object> pf = new Param1<Object>(null);
		if (!ctx.Read(pf))
			return;
		CarScript car = CarScript.Cast(pf.param1);
		float now = GetGame().GetTickTime();
		if (!car || now - m_EZ_QOL_LastFlipReq < EZ_QOL_FLIP_COOLDOWN)
			return;
		if (vector.Distance(car.GetPosition(), GetPosition()) > 5 || !EZ_QOL_Flip.CanFlip(this, car))
			return;
		m_EZ_QOL_LastFlipReq = now;
		NotificationSystem.SendNotificationToPlayerExtended(this, 4, "Flip vehicle", "Stand clear - flipping in 4 seconds...", "set:dayz_gui image:icon_info");
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(EZ_QOL_FlipKeyFinish, EZ_QOL_FLIP_KEY_DELAY_MS, false, car);
	}
}

class EZ_QOL_ActionFlipVehicleCB : ActionContinuousBaseCB
{
	override void CreateActionComponent()
	{
		m_ActionData.m_ActionComponent = new CAContinuousTime(EZ_QOL_Flip.TIME);
	}
}

class EZ_QOL_ActionFlipVehicle : ActionContinuousBase
{
	void EZ_QOL_ActionFlipVehicle()
	{
		m_CallbackClass = EZ_QOL_ActionFlipVehicleCB;
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_INTERACT;
		m_FullBody = true;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT | DayZPlayerConstants.STANCEMASK_CROUCH;
		m_Text = "#STR_EZ_QOL_FLIP_ACTION";
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINone;
		m_ConditionTarget = new CCTObject(UAMaxDistances.DEFAULT);
	}

	override typename GetInputType()
	{
		return ContinuousInteractActionInput;
	}

	override bool HasTarget()
	{
		return true;
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		return EZ_QOL_Flip.CanFlip(player, EZ_QOL_Flip.GetCar(target));
	}

	override void OnFinishProgressServer(ActionData action_data)
	{
		CarScript car = EZ_QOL_Flip.GetCar(action_data.m_Target);
		if (!EZ_QOL_Flip.CanFlip(action_data.m_Player, car))
			return;
		EZ_QOL_Flip.DoFlip(car);
	}
}
