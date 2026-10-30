// Base class for tablet apps. Interface matches ClearcREEk's EZ_QOL_DroneApp (GetId/GetTitle/GetLayout/OnOpen/OnClose/OnRpc).
class EZ_QOL_TabletApp
{
	string GetId() { return ""; }
	string GetTitle() { return ""; }
	string GetIcon() { return ""; }
	string GetLayout() { return ""; }
	void OnOpen(Widget root) {}
	void OnClose() {}
	void OnRpc(string fn, ParamsReadContext ctx) {}
	void Update(float dt) {}
	// return true if handled
	bool OnClick(Widget w) { return false; }
}
