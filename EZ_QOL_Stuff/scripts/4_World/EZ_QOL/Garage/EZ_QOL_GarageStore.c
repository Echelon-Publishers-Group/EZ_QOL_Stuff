// File layer around ExpansionEntityStorageModule (Expansion public API, used as a dependency).
// Files: $profile:EZ_QOL/Garage/<id>.bin (+ <id>\ subfolder Expansion may create), <id>.bak, <id>.json sidecar.
class EZ_QOL_GarageSidecar
{
	string VehType;
	int ItemCount;
	int KeyA; int KeyB; int KeyC; int KeyD;
	int LockState;
	float SizeX; float SizeY; float SizeZ;
}

class EZ_QOL_GarageStore
{
	static string Bin(string id)     { return EZ_QOL_GARAGE_DIR + "/" + id + ".bin"; }
	static string Bak(string id)     { return EZ_QOL_GARAGE_DIR + "/" + id + ".bak"; }
	static string Sidecar(string id) { return EZ_QOL_GARAGE_DIR + "/" + id + ".json"; }

	static int CountItems(EntityAI e)
	{
		array<EntityAI> items = new array<EntityAI>;
		e.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
		return items.Count() - 1; // minus the vehicle itself
	}

	static vector VehicleSize(EntityAI e)
	{
		vector mm[2];
		if (!e.GetCollisionBox(mm))
			return "0 0 0";
		return mm[1] - mm[0];
	}

	static bool Store(CarScript car, string id, out EZ_QOL_GarageSidecar sc)
	{
		sc = new EZ_QOL_GarageSidecar();
		sc.VehType = car.GetType();
		sc.ItemCount = CountItems(car);
		vector size = VehicleSize(car);
		sc.SizeX = size[0]; sc.SizeY = size[1]; sc.SizeZ = size[2];
		ExpansionVehicle ev = car.GetExpansionVehicle();
		if (ev)
		{
			ev.GetMasterKeyPersistentID(sc.KeyA, sc.KeyB, sc.KeyC, sc.KeyD);
			sc.LockState = ev.GetLockState();
		}
		JsonFileLoader<EZ_QOL_GarageSidecar>.JsonSaveFile(Sidecar(id), sc);

		if (!ExpansionEntityStorageModule.SaveToFile(car, Bin(id)))
		{
			EZ_QOL_Log.Err("SaveToFile failed for " + id + " (" + sc.VehType + ")");
			DeleteFile(Bin(id));
			return false;
		}
		if (!FileExist(Bin(id)))
		{
			EZ_QOL_Log.Err("SaveToFile returned true but " + Bin(id) + " is missing");
			return false;
		}
		return true;
	}

	static EZ_QOL_GarageSidecar LoadSidecar(string id)
	{
		EZ_QOL_GarageSidecar sc = new EZ_QOL_GarageSidecar();
		if (FileExist(Sidecar(id)))
			JsonFileLoader<EZ_QOL_GarageSidecar>.JsonLoadFile(Sidecar(id), sc);
		return sc;
	}

	// Copies .bin to .bak first because RestoreFromFile deletes the .bin on success.
	static EntityAI Restore(string id, vector pos, vector ori)
	{
		if (!FileExist(Bin(id)))
			return null;
		CopyFile(Bin(id), Bak(id));
		EntityAI ent;
		if (!ExpansionEntityStorageModule.RestoreFromFile(Bin(id), ent))
		{
			EZ_QOL_Log.Err("RestoreFromFile failed for " + id);
			if (!FileExist(Bin(id)) && FileExist(Bak(id)))
				CopyFile(Bak(id), Bin(id));
			return null;
		}
		if (!ent)
			return null;

		// restored at its old world position; move into this garage. UNVERIFIED for physics cars: may need a frame delay.
		ent.SetPosition(pos);
		ent.SetOrientation(ori);
		CarScript car;
		if (Class.CastTo(car, ent))
		{
			SetVelocity(car, vector.Zero);
			dBodySetAngularVelocity(car, vector.Zero);
			EZ_QOL_GarageSidecar sc = LoadSidecar(id);
			ExpansionVehicle ev = car.GetExpansionVehicle();
			if (ev && sc.KeyA != 0 && !ev.HasKey())
			{
				ev.SetMasterKeyPersistentID(sc.KeyA, sc.KeyB, sc.KeyC, sc.KeyD);
				ev.SetLockState(sc.LockState);
				EZ_QOL_Log.Info("Re-applied key ids for " + id);
			}
			int after = CountItems(car);
			if (after != sc.ItemCount)
				EZ_QOL_Log.Warn("Item count for " + id + ": stored " + sc.ItemCount + ", restored " + after + " (rest spilled to ground or lost)");
		}
		return ent;
	}

	static void Finish(string id)
	{
		DeleteFile(Bak(id));
		DeleteFile(Bin(id));
		DeleteFile(Sidecar(id));
	}

	static void MoveTo(string id, string dir)
	{
		TStringArray exts = {".bin", ".bak", ".json"};
		foreach (string ext : exts)
		{
			string src = EZ_QOL_GARAGE_DIR + "/" + id + ext;
			if (FileExist(src))
			{
				CopyFile(src, dir + "/" + id + ext);
				DeleteFile(src);
			}
		}
	}
}
