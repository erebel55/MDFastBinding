// Copyright Dylan Dumesnil. All Rights Reserved.

#pragma once

#include "Containers/Union.h"
#include "MDFastBindingMemberReference.h"
#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 3
#include "FieldNotificationId.h"
#else
#include "FieldNotification/FieldId.h"
#endif
#include "UObject/UnrealType.h"
#include "UObject/WeakFieldPtr.h"

#include "MDFastBindingFieldPath.generated.h"

DECLARE_DELEGATE_RetVal_OneParam(void*, FMDGetFieldPathOwner, UObject*);
DECLARE_DELEGATE_RetVal(UStruct*, FMDGetFieldPathOwnerStruct);
DECLARE_DELEGATE_RetVal_OneParam(bool, FMDFilterFieldPathField, const FFieldVariant&);

// Wraps FFieldVariant to weakly hold the field
struct FMDFastBindingWeakFieldVariant
{
public:
	FMDFastBindingWeakFieldVariant(const FField* InField)
		: WeakField(InField)
	{
	}

	FMDFastBindingWeakFieldVariant(UField* InField)
		: WeakField(InField)
	{
	}

	bool IsFieldValid() const
	{
		if (WeakField.HasSubtype<TWeakFieldPtr<const FField>>())
		{
			return WeakField.GetSubtype<TWeakFieldPtr<const FField>>().IsValid();
		}

		if (WeakField.HasSubtype<TWeakObjectPtr<UField>>())
		{
			return WeakField.GetSubtype<TWeakObjectPtr<UField>>().IsValid();
		}

		return false;
	}

	const FField* ToField() const
	{
		return WeakField.HasSubtype<TWeakFieldPtr<const FField>>() ? WeakField.GetSubtype<TWeakFieldPtr<const FField>>().Get() : nullptr;
	}

	UObject* ToUObject() const
	{
		return WeakField.HasSubtype<TWeakObjectPtr<UField>>() ? WeakField.GetSubtype<TWeakObjectPtr<UField>>().Get() : nullptr;
	}

	FFieldVariant GetFieldVariant() const
	{
		if (WeakField.HasSubtype<TWeakFieldPtr<const FField>>())
		{
			return FFieldVariant(WeakField.GetSubtype<TWeakFieldPtr<const FField>>().Get());
		}

		if (WeakField.HasSubtype<TWeakObjectPtr<UField>>())
		{
			return FFieldVariant(WeakField.GetSubtype<TWeakObjectPtr<UField>>().Get());
		}

		return {};
	}

private:
	TUnion<TWeakObjectPtr<UField>, TWeakFieldPtr<const FField>> WeakField;
};

/**
 *
 */
USTRUCT()
struct MDFASTBINDING_API FMDFastBindingFieldPath
{
	GENERATED_BODY()

public:
	~FMDFastBindingFieldPath();

	bool BuildPath();
	TArray<FFieldVariant> GetFieldPath();
	const TArray<FMDFastBindingWeakFieldVariant>& GetWeakFieldPath();

	// Returns a tuple containing the leaf property in the path (or return value property if a function) and a pointer to the value,
	// with an optional out param to retrieve the container that holds the leaf property
	TTuple<const FProperty*, void*> ResolvePath(UObject* SourceObject);
	TTuple<const FProperty*, void*> ResolvePath(UObject* SourceObject, void*& OutContainer);
	TTuple<const FProperty*, void*> ResolvePathFromRootObject(UObject* RootObject, void*& OutContainer);
	TTuple<const FProperty*, void*> ResolvePathFromRootObject(void* RootObjectPtr, void*& OutContainer);

	FFieldVariant GetLeafField();
	UE::FieldNotification::FFieldId GetLeafFieldId();
	const FProperty* GetLeafProperty();
	bool IsLeafFunction();

	bool IsPropertyValidForPath(const FProperty& Prop) const;
	bool IsFunctionValidForPath(const UFunction& Func) const;

	UStruct* GetPathOwnerStruct() const;

	FString ToString() const;

#if WITH_EDITOR
	void OnVariableRenamed(UClass* VariableClass, const FName& OldVariableName, const FName& NewVariableName);
#endif

	// Should be the FProperty-stored value (void* to struct or UObject**)
	FMDGetFieldPathOwner OwnerGetter;
	FMDGetFieldPathOwnerStruct OwnerStructGetter;
	FMDFilterFieldPathField FieldFilter;

	// Set to true if you're going to be setting the value of the property
	bool bOnlyAllowBlueprintReadWriteProperties = false;

	// Set to false if you only want to be able to select top-level properties
	bool bAllowSubProperties = true;

	// Set to false if you only want to allow properties in your field path
	bool bAllowGetterFunctions = true;

	UPROPERTY(meta = (DeprecatedProperty))
	TArray<FName> FieldPath;

	UPROPERTY(EditAnywhere, Category = "Bindings")
	TArray<FMDFastBindingMemberReference> FieldPathMembers;

private:
	void* InitAndGetFunctionMemory(const UFunction* Func);
	void CleanupFunctionMemory();

	void* InitAndGetPropertyMemory(const FProperty* Property);
	void CleanupPropertyMemory();

	void FixupFieldPath();

#if WITH_EDITORONLY_DATA
	TOptional<uint64> LastFrameUpdatedPath;
#endif

	TArray<FMDFastBindingWeakFieldVariant> CachedPath;
	TMap<TWeakObjectPtr<const UFunction>, void*> FunctionMemory;
	TMap<TWeakFieldPtr<FProperty>, void*> PropertyMemory;
};
