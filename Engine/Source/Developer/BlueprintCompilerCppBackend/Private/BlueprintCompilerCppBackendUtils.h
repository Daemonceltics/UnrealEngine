// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma  once

#include "BlueprintCompilerCppBackendGatherDependencies.h"

struct FCodeText
{
	FString Indent;
	FString Result;

	void IncreaseIndent()
	{
		Indent += TEXT("\t");
	}

	void DecreaseIndent()
	{
		Indent.RemoveFromEnd(TEXT("\t"));
	}

	void AddLine(const FString& Line)
	{
		Result += FString::Printf(TEXT("%s%s\n"), *Indent, *Line);
	}
};

struct FEmitterLocalContext
{
	enum class EClassSubobjectList
	{
		ComponentTemplates,
		Timelines,
		DynamicBindingObjects,
		MiscConvertedSubobjects,
	};

	enum class EGeneratedCodeType
	{
		SubobjectsOfClass,
		CommonConstructor,
		Regular,
	};

	EGeneratedCodeType CurrentCodeType;

private:
	int32 LocalNameIndexMax;

	//ConstructorOnly Local Names
	TMap<UObject*, FString> ClassSubobjectsMap;
	//ConstructorOnly Local Names
	TMap<UObject*, FString> CommonSubobjectsMap;

	// Class subobjects
	TArray<UObject*> MiscConvertedSubobjects;
	TArray<UObject*> DynamicBindingObjects;
	TArray<UObject*> ComponentTemplates;
	TArray<UObject*> Timelines;

public:

	FCodeText Header;
	FCodeText Body;
	FCodeText* DefaultTarget;

	const FGatherConvertedClassDependencies& Dependencies;

	TMap<UFunction*, FString> MCDelegateSignatureToSCDelegateType;

	FEmitterLocalContext(const FGatherConvertedClassDependencies& InDependencies)
		: CurrentCodeType(EGeneratedCodeType::Regular)
		, LocalNameIndexMax(0)
		, DefaultTarget(&Body)
		, Dependencies(InDependencies)
	{}

	// CONSTRUCTOR FUNCTIONS

	static const TCHAR* ClassSubobjectListName(EClassSubobjectList ListType)
	{
		if (ListType == EClassSubobjectList::ComponentTemplates)
		{
			return TEXT("ComponentTemplates");
		}
		if (ListType == EClassSubobjectList::Timelines)
		{
			return TEXT("Timelines");
		}
		if (ListType == EClassSubobjectList::DynamicBindingObjects)
		{
			return TEXT("DynamicBindingObjects");
		}
		return TEXT("MiscConvertedSubobjects");
	}

	void RegisterClassSubobject(UObject* Object, EClassSubobjectList ListType)
	{
		ensure(CurrentCodeType == FEmitterLocalContext::EGeneratedCodeType::SubobjectsOfClass);
		if (ListType == EClassSubobjectList::ComponentTemplates)
		{
			ComponentTemplates.Add(Object);
		}
		else if (ListType == EClassSubobjectList::Timelines)
		{
			Timelines.Add(Object);
		}
		else if(ListType == EClassSubobjectList::DynamicBindingObjects)
		{
			DynamicBindingObjects.Add(Object);
		}
		else
		{
			MiscConvertedSubobjects.Add(Object);
		}
	}

	void AddClassSubObject_InConstructor(UObject* Object, const FString& NativeName)
	{
		ensure(CurrentCodeType == EGeneratedCodeType::SubobjectsOfClass);
		ensure(!ClassSubobjectsMap.Contains(Object));
		ClassSubobjectsMap.Add(Object, NativeName);
	}

	void AddCommonSubObject_InConstructor(UObject* Object, const FString& NativeName)
	{
		ensure(CurrentCodeType == EGeneratedCodeType::CommonConstructor);
		ensure(!CommonSubobjectsMap.Contains(Object));
		CommonSubobjectsMap.Add(Object, NativeName);
	}

	// UNIVERSAL FUNCTIONS

	UClass* GetFirstNativeOrConvertedClass(UClass* InClass) const
	{
		check(InClass);
		for (UClass* ItClass = InClass; ItClass; ItClass = ItClass->GetSuperClass())
		{
			auto BPGC = Cast<UBlueprintGeneratedClass>(ItClass);
			if (ItClass->HasAnyClassFlags(CLASS_Native) || (ensure(BPGC) && Dependencies.WillClassBeConverted(BPGC)))
			{
				return ItClass;
			}
		}
		check(false);
		return nullptr;
	}

	FString GenerateUniqueLocalName();

	UClass* GetCurrentlyGeneratedClass() const
	{
		return Cast<UClass>(Dependencies.GetActualStruct());
	}

	/** All objects (that can be referenced from other package) that will have a different path in cooked build
	(due to the native code generation), should be handled by this function */
	FString FindGloballyMappedObject(const UObject* Object, const UClass* ExpectedClass = nullptr, bool bLoadIfNotFound = false, bool bTryUsedAssetsList = true);

	// Functions needed for Unconverted classes
	FString ExportCppDeclaration(const UProperty* Property, EExportedDeclaration::Type DeclarationType, uint32 AdditionalExportCPPFlags, bool bSkipParameterName = false) const;
	FString ExportTextItem(const UProperty* Property, const void* PropertyValue) const;

	// AS FCodeText
	void IncreaseIndent()
	{
		DefaultTarget->IncreaseIndent();
	}

	void DecreaseIndent()
	{
		DefaultTarget->DecreaseIndent();
	}

	void AddLine(const FString& Line)
	{
		DefaultTarget->AddLine(Line);
	}

private:
	FEmitterLocalContext(const FEmitterLocalContext&) = delete;
};

struct FEmitHelper
{
	// bUInterface - use interface with "U" prefix, by default there is "I" prefix
	static FString GetCppName(const UField* Field, bool bUInterface = false);

	static void ArrayToString(const TArray<FString>& Array, FString& OutString, const TCHAR* Separator);

	static bool HasAllFlags(uint64 Flags, uint64 FlagsToCheck);

	static bool IsMetaDataValid(const FName Name, const FString& Value);

	static FString HandleRepNotifyFunc(const UProperty* Property);

	static bool MetaDataCanBeNative(const FName MetaDataName, const UField* Field);

	static FString HandleMetaData(const UField* Field, bool AddCategory = true, const TArray<FString>* AdditinalMetaData = nullptr);

	static TArray<FString> ProperyFlagsToTags(uint64 Flags, bool bIsClassProperty);

	static TArray<FString> FunctionFlagsToTags(uint64 Flags);

	static bool IsBlueprintNativeEvent(uint64 FunctionFlags);

	static bool IsBlueprintImplementableEvent(uint64 FunctionFlags);

	static FString EmitUFuntion(UFunction* Function, const TArray<FString>& AdditionalTags, const TArray<FString>& AdditionalMetaData);

	static int32 ParseDelegateDetails(FEmitterLocalContext& EmitterContext, UFunction* Signature, FString& OutParametersMacro, FString& OutParamNumberStr);

	static void EmitSinglecastDelegateDeclarations(FEmitterLocalContext& EmitterContext, const TArray<UDelegateProperty*>& Delegates);

	static void EmitSinglecastDelegateDeclarations_Inner(FEmitterLocalContext& EmitterContext, UFunction* Signature, const FString& TypeName);

	static void EmitMulticastDelegateDeclarations(FEmitterLocalContext& EmitterContext);

	static void EmitLifetimeReplicatedPropsImpl(FEmitterLocalContext& EmitterContext);

	static FString LiteralTerm(FEmitterLocalContext& EmitterContext, const FEdGraphPinType& Type, const FString& CustomValue, UObject* LiteralObject);

	static FString DefaultValue(FEmitterLocalContext& EmitterContext, const FEdGraphPinType& Type);

	static FString PinTypeToNativeType(const FEdGraphPinType& InType);

	static UFunction* GetOriginalFunction(UFunction* Function);

	static bool ShouldHandleAsNativeEvent(UFunction* Function);

	static bool ShouldHandleAsImplementableEvent(UFunction* Function);

	static bool GenerateAutomaticCast(FEmitterLocalContext& EmitterContext, const FEdGraphPinType& LType, const FEdGraphPinType& RType, FString& OutCastBegin, FString& OutCastEnd);

	static FString GenerateReplaceConvertedMD(UObject* Obj);

	static FString GetBaseFilename(const UObject* AssetObj);

	static FString ReplaceConvertedMetaData(UObject* Obj);

	static FString GetPCHFilename();

	static FString GetGameMainHeaderFilename();

	static FString GenerateGetPropertyByName(FEmitterLocalContext& EmitterContext, const UProperty* Property);

	static FString AccessInaccessibleProperty(FEmitterLocalContext& EmitterContext, const UProperty* Property
		, const FString& ContextStr, const FString& ContextAdressOp, int32 StaticArrayIdx = 0);

	// This code works properly as long, as all fields in structures are UProperties!
	static FString AccessInaccessiblePropertyUsingOffset(FEmitterLocalContext& EmitterContext, const UProperty* Property
		, const FString& ContextStr, const FString& ContextAdressOp, int32 StaticArrayIdx = 0);

};

struct FNonativeComponentData;

struct FEmitDefaultValueHelper
{
	static void GenerateGetDefaultValue(const UUserDefinedStruct* Struct, FEmitterLocalContext& EmitterContext);

	static void GenerateConstructor(FEmitterLocalContext& Context);

	enum class EPropertyAccessOperator
	{
		None, // for self scope, this
		Pointer,
		Dot,
	};

	// OuterPath ends context/outer name (or empty, if the scope is "this")
	static void OuterGenerate(FEmitterLocalContext& Context, const UProperty* Property, const FString& OuterPath, const uint8* DataContainer, const uint8* OptionalDefaultDataContainer, EPropertyAccessOperator AccessOperator, bool bAllowProtected = false);


	// PathToMember ends with variable name
	static void InnerGenerate(FEmitterLocalContext& Context, const UProperty* Property, const FString& PathToMember, const uint8* ValuePtr, const uint8* DefaultValuePtr, bool bWithoutFirstConstructionLine = false);

	// Creates the subobject (of class) returns it's native local name, 
	// returns empty string if cannot handle
	static FString HandleClassSubobject(FEmitterLocalContext& Context, UObject* Object, FEmitterLocalContext::EClassSubobjectList ListOfSubobjectsTyp, bool bCreate, bool bInitilize);

private:
	// Returns native term, 
	// returns empty string if cannot handle
	static FString HandleSpecialTypes(FEmitterLocalContext& Context, const UProperty* Property, const uint8* ValuePtr);

	static FString HandleNonNativeComponent(FEmitterLocalContext& Context, const USCS_Node* Node, TSet<const UProperty*>& OutHandledProperties
		, TArray<FString>& NativeCreatedComponentProperties, const USCS_Node* ParentNode, TArray<FNonativeComponentData>& ComponentsToInit);

	static FString HandleInstancedSubobject(FEmitterLocalContext& Context, UObject* Object, bool bCreateInstance = true, bool bSkipEditorOnlyCheck = false);
};

struct FBackendHelperUMG
{
	static void WidgetFunctionsInHeader(FEmitterLocalContext& Context);

	static void AdditionalHeaderIncludeForWidget(FEmitterLocalContext& EmitterContext);

	// these function should use the same context as Constructor
	static void CreateClassSubobjects(FEmitterLocalContext& Context, bool bCreate, bool bInitialize);
	static void EmitWidgetInitializationFunctions(FEmitterLocalContext& Context);

};

struct FBackendHelperAnim
{
	static void AddHeaders(FEmitterLocalContext& EmitterContext);

	static void CreateAnimClassData(FEmitterLocalContext& Context);
};