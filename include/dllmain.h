// dllmain.h : Declaration of module class.

class CAgenticAIOnWordModule : public ATL::CAtlDllModuleT< CAgenticAIOnWordModule >
{
public :
	DECLARE_LIBID(LIBID_AgenticAIOnWordLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_AGENTICAIONWORD, "{cddbcd6c-2230-4b77-bd3c-2d09ed326605}")
};

extern class CAgenticAIOnWordModule _AtlModule;
