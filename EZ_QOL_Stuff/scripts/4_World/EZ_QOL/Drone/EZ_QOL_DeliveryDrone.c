// Scripted, server-moved drone. No physics.
// Server: SetPosition every tick (authority) + netsynced position/velocity snapshot at ~4 Hz.
// Client: extrapolates from the snapshot and eases toward it every frame (position smoothing),
//         spins rotors, plays loop sound, light on memory point light_pos.
class EZ_QOL_DroneLight: PointLightBase
{
	void EZ_QOL_DroneLight()
	{
		SetVisibleDuringDaylight(true);
		SetRadiusTo(6);
		SetBrightnessTo(2.5);
		SetCastShadow(false);
		SetDiffuseColor(1.0, 0.25, 0.2);
		SetFlareVisible(false);
	}
}

class EZ_QOL_DeliveryDrone: House
{
	protected bool m_EZ_RotorsOn;
	protected bool m_EZ_CargoAttached = true;
	// net snapshot
	protected float m_EZ_NetPX;
	protected float m_EZ_NetPY;
	protected float m_EZ_NetPZ;
	protected float m_EZ_NetVX;
	protected float m_EZ_NetVY;
	protected float m_EZ_NetVZ;
	protected float m_EZ_NetYaw;
	protected int m_EZ_LastSyncMs;

	// client
	protected float m_EZ_RotorPhase;
	protected EffectSound m_EZ_LoopSound;
	protected EZ_QOL_DroneLight m_EZ_Light;
	protected bool m_EZ_ClientStarted;
	protected float m_EZ_SnapAge;
	protected bool m_EZ_HasSnap;

	void EZ_QOL_DeliveryDrone()
	{
		RegisterNetSyncVariableBool("m_EZ_RotorsOn");
		RegisterNetSyncVariableBool("m_EZ_CargoAttached");
		RegisterNetSyncVariableFloat("m_EZ_NetPX", 0, 0, 2);
		RegisterNetSyncVariableFloat("m_EZ_NetPY", 0, 0, 2);
		RegisterNetSyncVariableFloat("m_EZ_NetPZ", 0, 0, 2);
		RegisterNetSyncVariableFloat("m_EZ_NetVX", 0, 0, 2);
		RegisterNetSyncVariableFloat("m_EZ_NetVY", 0, 0, 2);
		RegisterNetSyncVariableFloat("m_EZ_NetVZ", 0, 0, 2);
		RegisterNetSyncVariableFloat("m_EZ_NetYaw", 0, 0, 1);
		if (!GetGame().IsDedicatedServer())
			SetEventMask(EntityEvent.FRAME);
	}

	void ~EZ_QOL_DeliveryDrone()
	{
		EZ_QOL_StopClientFx();
	}

	// ---------------- server ----------------
	void EZ_QOL_SetRotors(bool on)
	{
		m_EZ_RotorsOn = on;
		SetSynchDirty();
	}

	void EZ_QOL_SetCargoAttached(bool attached)
	{
		m_EZ_CargoAttached = attached;
		EZ_QOL_ApplyCargoPhase();
		SetSynchDirty();
	}

	// Called by the manager every tick with the new authoritative pose and velocity.
	void EZ_QOL_ServerMove(vector pos, vector vel, float yaw)
	{
		SetPosition(pos);
		SetOrientation(Vector(yaw, 0, 0));
		int now = GetGame().GetTime();
		if (now - m_EZ_LastSyncMs >= 250)
		{
			m_EZ_LastSyncMs = now;
			m_EZ_NetPX = pos[0];
			m_EZ_NetPY = pos[1];
			m_EZ_NetPZ = pos[2];
			m_EZ_NetVX = vel[0];
			m_EZ_NetVY = vel[1];
			m_EZ_NetVZ = vel[2];
			m_EZ_NetYaw = yaw;
			SetSynchDirty();
		}
	}

	// Drop position in world space (memory point cargo_drop, fallback cargo_center, then origin)
	vector EZ_QOL_GetCargoDropWorld()
	{
		if (MemoryPointExists("cargo_drop"))
			return ModelToWorld(GetMemoryPointPos("cargo_drop"));
		if (MemoryPointExists("cargo_center"))
			return ModelToWorld(GetMemoryPointPos("cargo_center"));
		return GetPosition();
	}

	protected void EZ_QOL_ApplyCargoPhase()
	{
		if (m_EZ_CargoAttached)
			SetAnimationPhase("cargo_attached", EZ_QOL_DRONE_CARGO_PHASE_ATTACHED);
		else
			SetAnimationPhase("cargo_attached", EZ_QOL_DRONE_CARGO_PHASE_DROPPED);
	}

	// ---------------- client ----------------
	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();
		if (m_EZ_RotorsOn && !m_EZ_ClientStarted)
			EZ_QOL_StartClientFx();
		if (!m_EZ_RotorsOn && m_EZ_ClientStarted)
			EZ_QOL_StopClientFx();
		EZ_QOL_ApplyCargoPhase();
		m_EZ_SnapAge = 0;
		m_EZ_HasSnap = true;
	}

	protected void EZ_QOL_StartClientFx()
	{
		m_EZ_ClientStarted = true;
		PlaySoundSetLoop(m_EZ_LoopSound, "EZ_QOL_DroneLoop_SoundSet", 0.3, 0.3);
		EffectSound takeoff = SEffectManager.PlaySoundOnObject("EZ_QOL_DroneTakeoff_SoundSet", this);
		if (takeoff)
			takeoff.SetAutodestroy(true);
		if (MemoryPointExists("light_pos"))
		{
			m_EZ_Light = EZ_QOL_DroneLight.Cast(ScriptedLightBase.CreateLight(EZ_QOL_DroneLight, "0 0 0"));
			if (m_EZ_Light)
				m_EZ_Light.AttachOnMemoryPoint(this, "light_pos");
		}
	}

	protected void EZ_QOL_StopClientFx()
	{
		m_EZ_ClientStarted = false;
		if (m_EZ_LoopSound)
			StopSoundSet(m_EZ_LoopSound);
		if (m_EZ_Light)
		{
			m_EZ_Light.Destroy();
			m_EZ_Light = null;
		}
	}

	override void EOnFrame(IEntity other, float timeSlice)
	{
		if (!m_EZ_RotorsOn)
			return;

		// rotors: phase loops 0..1 (model.cfg sourceAddress loop)
		m_EZ_RotorPhase += timeSlice * 8.0;
		if (m_EZ_RotorPhase > 1.0)
			m_EZ_RotorPhase -= Math.Floor(m_EZ_RotorPhase);
		SetAnimationPhase("rotor", m_EZ_RotorPhase);

		// position smoothing: extrapolate snapshot, ease current pose toward it
		if (!m_EZ_HasSnap)
			return;
		m_EZ_SnapAge += timeSlice;
		float age = Math.Min(m_EZ_SnapAge, 1.0);
		vector predicted = Vector(m_EZ_NetPX + m_EZ_NetVX * age, m_EZ_NetPY + m_EZ_NetVY * age, m_EZ_NetPZ + m_EZ_NetVZ * age);
		vector cur = GetPosition();
		float k = Math.Clamp(timeSlice * 8.0, 0, 1);
		vector smooth = cur + (predicted - cur) * k;
		if (vector.Distance(cur, predicted) > 30)
			smooth = predicted; // teleport on big error
		SetPosition(smooth);
		SetOrientation(Vector(m_EZ_NetYaw, 0, 0));
	}
}
