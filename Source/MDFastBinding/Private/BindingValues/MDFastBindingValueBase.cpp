﻿// Copyright Dylan Dumesnil. All Rights Reserved.

#include "BindingValues/MDFastBindingValueBase.h"

#include "Misc/App.h"

void UMDFastBindingValueBase::BeginDestroy()
{
	Super::BeginDestroy();

	if (HasCachedValue())
	{
		FMemory::Free(CachedValue.Value);
		CachedValue.Value = nullptr;
	}
}

void UMDFastBindingValueBase::InitializeValue(UObject* SourceObject)
{
	SetupBindingItems_Internal();

	for (FMDFastBindingItem& BindingItem : BindingItems)
	{
		if (BindingItem.Value != nullptr)
		{
			BindingItem.Value->InitializeValue(SourceObject);
		}
		else
		{
			// Cache default values on initializations
			bool bDidUpdate = false;
			BindingItem.GetValue(SourceObject, bDidUpdate);
		}
	}

	InitializeValue_Internal(SourceObject);
}

void UMDFastBindingValueBase::TerminateValue(UObject* SourceObject)
{
	TerminateValue_Internal(SourceObject);

	for (const FMDFastBindingItem& BindingItem : BindingItems)
	{
		if (BindingItem.Value != nullptr)
		{
			BindingItem.Value->TerminateValue(SourceObject);
		}
	}
}

TTuple<const FProperty*, void*> UMDFastBindingValueBase::GetValue(UObject* SourceObject, bool& OutDidUpdate)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(__FUNCTION__);
	TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*GetName());

	OutDidUpdate = false;

	if (CheckCachedNeedsUpdate())
	{
#if WITH_EDITORONLY_DATA
		LastTimeNodeRan = FApp::GetCurrentTime();
#endif
		const TTuple<const FProperty*, void*> Value = GetValue_Internal(SourceObject);
		if (Value.Key == nullptr || Value.Value == nullptr)
		{
			return Value;
		}

		if (CachedValue.Key == nullptr || CachedValue.Value == nullptr)
		{
			CachedValue.Key = Value.Key;
			CachedValue.Value = FMemory::Malloc(CachedValue.Key->GetSize(), CachedValue.Key->GetMinAlignment());
			CachedValue.Key->InitializeValue(CachedValue.Value);
			CachedValue.Key->CopyCompleteValue(CachedValue.Value, Value.Value);
			OutDidUpdate = true;
		}
		else if (!CachedValue.Key->Identical(CachedValue.Value, Value.Value))
		{
			CachedValue.Key->CopyCompleteValue(CachedValue.Value, Value.Value);
			OutDidUpdate = true;
		}

		MarkObjectClean();
	}

	return CachedValue;
}

const FMDFastBindingItem* UMDFastBindingValueBase::GetOwningBindingItem() const
{
	if (const UMDFastBindingObject* OuterObject = Cast<UMDFastBindingObject>(GetOuter()))
	{
		return OuterObject->FindBindingItemWithValue(this);
	}

	return nullptr;
}
