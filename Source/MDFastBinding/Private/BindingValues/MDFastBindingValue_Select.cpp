﻿
#include "BindingValues/MDFastBindingValue_Select.h"

#include "MDFastBindingHelpers.h"

#define LOCTEXT_NAMESPACE "MDFastBindingValue_Select"

namespace MDFastBindingValue_Select_Private
{
	const FName SelectValueInputName = TEXT("Value");
	const FName FallbackResultInputName = TEXT("Default");
	const FName MappingItemListName = TEXT("Mappings");
	const FName FromValueItemName = TEXT("Select Value");
	const FName ToValueItemName = TEXT("Result Value");
	const FName FalseItemName = TEXT("False");
	const FName TrueItemName = TEXT("True");
}

const FProperty* UMDFastBindingValue_Select::GetOutputProperty()
{
	return ResolveOutputProperty();
}

bool UMDFastBindingValue_Select::HasUserExtendablePinList() const
{
	const FProperty* SelectValueProp = ResolveBindingItemProperty(MDFastBindingValue_Select_Private::SelectValueInputName);
	return SelectValueProp != nullptr && !(SelectValueProp->IsA<FBoolProperty>() || SelectValueProp->IsA<FEnumProperty>());
}

TTuple<const FProperty*, void*> UMDFastBindingValue_Select::GetValue_Internal(UObject* SourceObject)
{
	bool bDidUpdate = false;
	TTuple<const FProperty*, void*> InputValue = GetBindingItemValue(SourceObject, MDFastBindingValue_Select_Private::SelectValueInputName, bDidUpdate);
	if (InputValue.Key == nullptr || InputValue.Value == nullptr)
	{
		return {};
	}

	if (const FBoolProperty* BoolProp = CastField<const FBoolProperty>(InputValue.Key))
	{
		static const bool TrueValue = true;
		if (BoolProp->Identical(&TrueValue, InputValue.Value, 0))
		{
			return GetBindingItemValue(SourceObject, MDFastBindingValue_Select_Private::TrueItemName, bDidUpdate);
		}
		else
		{
			return GetBindingItemValue(SourceObject, MDFastBindingValue_Select_Private::FalseItemName, bDidUpdate);
		}
	}
	else if (const FEnumProperty* EnumProp = CastField<const FEnumProperty>(InputValue.Key))
	{
		if (const FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty())
		{
			const int64 Value = UnderlyingProp->GetSignedIntPropertyValue(InputValue.Value);
			if (const FName* PinName = EnumValueToPinNameMap.Find(Value))
			{
				return GetBindingItemValue(SourceObject, *PinName, bDidUpdate);
			}
		}
	}
	else
	{
		for (FMDFastBindingItem& BindingItem : BindingItems)
		{
			if (BindingItem.ExtendablePinListIndex != INDEX_NONE && BindingItem.ExtendablePinListNameBase == MDFastBindingValue_Select_Private::FromValueItemName)
			{
				const TTuple<const FProperty*, void*> ItemValue = BindingItem.GetValue(SourceObject, bDidUpdate);
				if (FMDFastBindingHelpers::ArePropertyValuesEqual(ItemValue.Key, ItemValue.Value, InputValue.Key, InputValue.Value))
				{
					const FName& ResultValueName = FindOrCreateExtendableItemName(MDFastBindingValue_Select_Private::ToValueItemName, BindingItem.ExtendablePinListIndex);
					return GetBindingItemValue(SourceObject, ResultValueName, bDidUpdate);
				}
			}
		}

		return GetBindingItemValue(SourceObject, MDFastBindingValue_Select_Private::FallbackResultInputName, bDidUpdate);
	}

	return {};
}

void UMDFastBindingValue_Select::SetupBindingItems()
{
	EnsureBindingItemExists(MDFastBindingValue_Select_Private::SelectValueInputName, nullptr
		, LOCTEXT("ValueInputDescription", "Based on the input to this pin, this node will select the output value."));

	TSet<FName> ExpectedNames = { MDFastBindingValue_Select_Private::SelectValueInputName };

#if WITH_EDITOR
	// Names that have a display name that don't match the enum name string for backwards compatibility
	TMap<FName, FName> EnumDisplayNamesForFixUp;
#endif

	const FProperty* SelectValueProp = ResolveBindingItemProperty(MDFastBindingValue_Select_Private::SelectValueInputName);
	const FProperty* OutputProp = ResolveOutputProperty();
	if (SelectValueProp != nullptr && SelectValueProp->IsA<FBoolProperty>())
	{
		ExpectedNames.Add(MDFastBindingValue_Select_Private::TrueItemName);
		ExpectedNames.Add(MDFastBindingValue_Select_Private::FalseItemName);
		EnsureBindingItemExists(MDFastBindingValue_Select_Private::TrueItemName, OutputProp, FText::GetEmpty(), true).ForceDisplayItemName();
		EnsureBindingItemExists(MDFastBindingValue_Select_Private::FalseItemName, OutputProp, FText::GetEmpty(), true).ForceDisplayItemName();
	}
	else if (const FEnumProperty* EnumProp = CastField<const FEnumProperty>(SelectValueProp))
	{
		UEnum* Enum = EnumProp->GetEnum();
		for (int32 EnumIndex = 0; EnumIndex < Enum->NumEnums() - 1; ++EnumIndex)
		{
#if WITH_EDITOR
			bool const bShouldBeHidden = Enum->HasMetaData(TEXT("Hidden"), EnumIndex) || Enum->HasMetaData(TEXT("Spacer"), EnumIndex);
			if (!bShouldBeHidden)
#endif
			{
				const int64 EnumValue = Enum->GetValueByIndex(EnumIndex);
				const FName EnumNameString = *Enum->GetNameStringByIndex(EnumIndex);
				EnumValueToPinNameMap.Add(EnumValue, EnumNameString);

#if WITH_EDITOR
				const FName EnumFriendlyName = *Enum->GetDisplayNameTextByIndex(EnumIndex).ToString();
				if (EnumFriendlyName != EnumNameString)
				{
					EnumDisplayNamesForFixUp.Add(EnumFriendlyName, EnumNameString);
				}
#endif

				ExpectedNames.Add(EnumNameString);
				EnsureBindingItemExists(EnumNameString, OutputProp, FText::GetEmpty(), true).ForceDisplayItemName();
			}
		}
	}
	else
	{
		ExpectedNames.Add(MDFastBindingValue_Select_Private::FallbackResultInputName);
		EnsureBindingItemExists(MDFastBindingValue_Select_Private::FallbackResultInputName, OutputProp
			, LOCTEXT("FallbackInputDescription", "If the input value doesn't map to any of the inputs, this value is output instead."), true).ForceDisplayItemName();

		ExtendablePinListCount = FMath::Max(1, ExtendablePinListCount);
	}

	for (auto It = BindingItems.CreateIterator(); It; ++It)
	{
		if (It->ExtendablePinListIndex == INDEX_NONE && !ExpectedNames.Contains(It->ItemName))
		{
#if WITH_EDITOR
			// Does this item have a corresponding display name that may need a fixup?
			if (EnumDisplayNamesForFixUp.Contains(It->ItemName))
			{
				// Copy the old data over if we have an item from before the fix
				if (FMDFastBindingItem* NewItem = BindingItems.FindByKey(EnumDisplayNamesForFixUp[It->ItemName]))
				{
					NewItem->Value = It->Value;
					It->Value = nullptr;

					NewItem->DefaultObject = It->DefaultObject;
					NewItem->DefaultString = It->DefaultString;
					NewItem->DefaultText = It->DefaultText;
					NewItem->ExtendablePinListIndex = It->ExtendablePinListIndex;
					NewItem->ExtendablePinListNameBase = It->ExtendablePinListNameBase;
					It.RemoveCurrent();
					continue;
				}
			}
#endif
#if WITH_EDITORONLY_DATA
			OrphanBindingItem(It->Value);
#endif
			It.RemoveCurrent();
		}
	}
}

void UMDFastBindingValue_Select::SetupExtendablePinBindingItem(int32 ItemIndex)
{
	const FProperty* SelectValueProp = ResolveBindingItemProperty(MDFastBindingValue_Select_Private::SelectValueInputName);
	EnsureExtendableBindingItemExists(MDFastBindingValue_Select_Private::FromValueItemName, SelectValueProp, FText::GetEmpty(), ItemIndex).ForceDisplayItemName();
	EnsureExtendableBindingItemExists(MDFastBindingValue_Select_Private::ToValueItemName, ResolveOutputProperty(), FText::GetEmpty(), ItemIndex).ForceDisplayItemName();
}

#if WITH_EDITORONLY_DATA
FText UMDFastBindingValue_Select::GetDisplayName()
{
	const FProperty* InputValueProp = ResolveBindingItemProperty(MDFastBindingValue_Select_Private::SelectValueInputName);
	const FProperty* OutputValueProp = ResolveOutputProperty();
	if (InputValueProp != nullptr && OutputValueProp != nullptr)
	{
		return FText::Format(INVTEXT("{0} -> {1}")
			, FText::FromString(FMDFastBindingHelpers::PropertyToString(*InputValueProp))
			, FText::FromString(FMDFastBindingHelpers::PropertyToString(*OutputValueProp)));
	}

	return Super::GetDisplayName();
}
#endif

#if WITH_EDITOR
EDataValidationResult UMDFastBindingValue_Select::IsDataValid(TArray<FText>& ValidationErrors)
{
	return Super::IsDataValid(ValidationErrors);
}
#endif

const FProperty* UMDFastBindingValue_Select::ResolveOutputProperty()
{
#if !WITH_EDITOR
	if (ResolvedOutputProperty.IsValid())
	{
		return ResolvedOutputProperty.Get();
	}
#endif

	if (const FMDFastBindingItem* OwningItem = GetOwningBindingItem())
	{
		if (const FProperty* OwningItemProp = OwningItem->ItemProperty.Get())
		{
			ResolvedOutputProperty = OwningItemProp;
			return OwningItemProp;
		}
	}

	if (const FProperty* FallbackValueProp = GetBindingItemValueProperty(MDFastBindingValue_Select_Private::FallbackResultInputName))
	{
		ResolvedOutputProperty = FallbackValueProp;
		return FallbackValueProp;
	}

	const FName& FirstToValueInputName = FindOrCreateExtendableItemName(MDFastBindingValue_Select_Private::ToValueItemName, 0);
	if (const FProperty* FirstToValueProp = GetBindingItemValueProperty(FirstToValueInputName))
	{
		ResolvedOutputProperty = FirstToValueProp;
		return FirstToValueProp;
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE
