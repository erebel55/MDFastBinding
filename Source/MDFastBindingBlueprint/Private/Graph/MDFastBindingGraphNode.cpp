#include "Graph/MDFastBindingGraphNode.h"

#include "BindingDestinations/MDFastBindingDestinationBase.h"
#include "BindingDestinations/MDFastBindingDestination_Function.h"
#include "BindingValues/MDFastBindingValueBase.h"
#include "BindingValues/MDFastBindingValue_Function.h"
#include "EdGraphSchema_K2.h"
#include "Graph/MDFastBindingGraph.h"
#include "Graph/SMDFastBindingGraphNodeWidget.h"
#include "K2Node_CallFunction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "MDFastBindingHelpers.h"
#include "MDFastBindingInstance.h"
#include "MDFastBindingObject.h"
#include "Misc/EngineVersionComparison.h"
#include "Styling/SlateStyleRegistry.h"

#define LOCTEXT_NAMESPACE "MDFastBindingGraphNode"

const FName UMDFastBindingGraphNode::OutputPinName = TEXT("Output");

UMDFastBindingGraphNode::UMDFastBindingGraphNode()
{
	bCanRenameNode = true;
}

void UMDFastBindingGraphNode::SetBindingObject(UMDFastBindingObject* InObject)
{
	BindingObject = InObject;
	if (InObject != nullptr)
	{
		NodePosX = InObject->NodePos.X;
		NodePosY = InObject->NodePos.Y;
		NodeComment = InObject->DevComment;
		bCommentBubbleVisible = InObject->bIsCommentBubbleVisible;
		NodeGuid = InObject->BindingObjectIdentifier;
	}
	else
	{
		CreateNewGuid();
	}
}

void UMDFastBindingGraphNode::SetBindingBeingDebugged(UMDFastBindingInstance* InBinding)
{
	BindingBeingDebugged = InBinding;
}

UMDFastBindingObject* UMDFastBindingGraphNode::GetBindingObjectBeingDebugged() const
{
	if (BindingObject != nullptr && BindingBeingDebugged.IsValid())
	{
		return BindingBeingDebugged->FindBindingObjectWithGUID(BindingObject->BindingObjectIdentifier);
	}

	return nullptr;
}

UMDFastBindingGraphNode* UMDFastBindingGraphNode::GetLinkedOutputNode() const
{
	const UEdGraphPin* OutputPin = FindPin(OutputPinName);
	if (OutputPin == nullptr || OutputPin->LinkedTo.Num() == 0)
	{
		return nullptr;
	}

	return Cast<UMDFastBindingGraphNode>(OutputPin->LinkedTo[0]->GetOwningNode());
}

void UMDFastBindingGraphNode::ClearConnection(const FName& PinName)
{
	if (UMDFastBindingObject* Object = BindingObject.Get())
	{
		Object->Modify();
		Object->ClearBindingItemValue(PinName);
	}
}

void UMDFastBindingGraphNode::OnMoved()
{
	if (UMDFastBindingObject* Object = BindingObject.Get())
	{
		Object->Modify();
		Object->NodePos = FIntPoint(NodePosX, NodePosY);
	}
}

void UMDFastBindingGraphNode::DeleteNode(const TSet<UObject*>& OrphanExclusionSet)
{
	if (UMDFastBindingObject* Object = BindingObject.Get())
	{
		if (UMDFastBindingDestinationBase* BindingDest = Cast<UMDFastBindingDestinationBase>(Object))
		{
			if (UMDFastBindingInstance* Binding = BindingDest->GetOuterBinding())
			{
				Binding->Modify();
				Binding->RemoveDestination(BindingDest);
				BindingDest->OrphanAllBindingItems(OrphanExclusionSet);
			}

			FBlueprintEditorUtils::MarkBlueprintAsModified(GetTypedOuter<UBlueprint>());
			return;
		}

		UEdGraphPin* OutputPin = FindPin(OutputPinName);
		if (OutputPin == nullptr || OutputPin->LinkedTo.Num() == 0)
		{
			// We're probably an orphan
			if (UMDFastBindingValueBase* ValueBase = Cast<UMDFastBindingValueBase>(Object))
			{
				if (UMDFastBindingInstance* Binding = ValueBase->GetOuterBinding())
				{
					Binding->Modify();
					Binding->RemoveOrphan(ValueBase);
					ValueBase->OrphanAllBindingItems(OrphanExclusionSet);
				}
			}

			FBlueprintEditorUtils::MarkBlueprintAsModified(GetTypedOuter<UBlueprint>());
			return;
		}

		UEdGraphPin* ConnectedPin = OutputPin->LinkedTo[0];
		if (ConnectedPin == nullptr)
		{
			return;
		}

		if (UMDFastBindingValueBase* BindingValue = Cast<UMDFastBindingValueBase>(Object))
		{
			BindingValue->Modify();
			BindingValue->OrphanAllBindingItems(OrphanExclusionSet);
		}

		if (UMDFastBindingGraphNode* ConnectedNode = Cast<UMDFastBindingGraphNode>(ConnectedPin->GetOwningNode()))
		{
			ConnectedNode->ClearConnection(ConnectedPin->GetFName());
		}

		FBlueprintEditorUtils::MarkBlueprintAsModified(GetTypedOuter<UBlueprint>());
	}
}

void UMDFastBindingGraphNode::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	if (UMDFastBindingObject* Object = BindingObject.Get())
	{
		if (FMDFastBindingItem* Item = Object->FindBindingItem(Pin->GetFName()))
		{
			Object->Modify();
			Item->DefaultObject = Pin->DefaultObject;
			Item->DefaultString = Pin->DefaultValue;
			Item->DefaultText = Pin->DefaultTextValue;

			FBlueprintEditorUtils::MarkBlueprintAsModified(GetTypedOuter<UBlueprint>());
			RefreshGraph();
		}
	}
}

void UMDFastBindingGraphNode::PrepareForCopying()
{
	Super::PrepareForCopying();

	if (const UMDFastBindingObject* Obj = GetBindingObject())
	{
		CopiedObject = DuplicateObject<UMDFastBindingObject>(Obj, this);
	}
}

FString UMDFastBindingGraphNode::GetPinMetaData(FName InPinName, FName InKey)
{
	if (const UMDFastBindingObject* Object = BindingObject.Get())
	{
		if (const FMDFastBindingItem* Item = Object->FindBindingItem(InPinName))
		{
			if (const FProperty* Prop = Item->ItemProperty.Get())
			{
				return Prop->GetMetaData(InKey);
			}
		}
	}

	return Super::GetPinMetaData(InPinName, InKey);
}

FText UMDFastBindingGraphNode::GetPinDisplayName(const UEdGraphPin* Pin) const
{
	return GetDefault<UEdGraphSchema_K2>()->GetPinDisplayName(Pin);
}

void UMDFastBindingGraphNode::CleanUpCopying()
{
	CopiedObject = nullptr;
}

void UMDFastBindingGraphNode::CleanUpPasting()
{
	CopiedObject = nullptr;
}

FText UMDFastBindingGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (UMDFastBindingDestinationBase* BindingDest = Cast<UMDFastBindingDestinationBase>(GetBindingObject()))
	{
		if (!BindingDest->IsActive())
		{
			static const FText InactiveFormat = LOCTEXT("InactiveDestinationNodeTitleFormat", "{0} (Inactive)");
			return FText::Format(InactiveFormat, BindingDest->DevName.IsEmptyOrWhitespace() ? BindingDest->GetClass()->GetDisplayNameText() : BindingDest->DevName);
		}
	}

	if (UMDFastBindingObject* Object = BindingObject.Get())
	{
		return Object->DevName.IsEmptyOrWhitespace() ? Object->GetClass()->GetDisplayNameText() : Object->DevName;
	}

	return Super::GetNodeTitle(TitleType);
}

TSharedPtr<SGraphNode> UMDFastBindingGraphNode::CreateVisualWidget()
{
	return SNew(SMDFastBindingGraphNodeWidget, this);
}

void UMDFastBindingGraphNode::OnRenameNode(const FString& NewName)
{
	if (UMDFastBindingObject* Object = BindingObject.Get())
	{
		Object->Modify();
		Object->DevName = FText::FromString(NewName);
	}
}

void UMDFastBindingGraphNode::OnUpdateCommentText(const FString& NewComment)
{
	if(!NodeComment.Equals(NewComment))
	{
		Modify();
		NodeComment	= NewComment;

		if (UMDFastBindingObject* Object = BindingObject.Get())
		{
			Object->Modify();
			Object->DevComment = NewComment;
		}
	}
}

void UMDFastBindingGraphNode::OnCommentBubbleToggled(bool bInCommentBubbleVisible)
{
	if (UMDFastBindingObject* Object = BindingObject.Get())
	{
		if (Object->bIsCommentBubbleVisible != bInCommentBubbleVisible)
		{
			Object->Modify();
			Object->bIsCommentBubbleVisible = bInCommentBubbleVisible;
		}
	}
}

FLinearColor UMDFastBindingGraphNode::GetNodeTitleColor() const
{
	const ISlateStyle* FastBindingStyle = FSlateStyleRegistry::FindSlateStyle(TEXT("MDFastBindingEditorStyle"));
	if (FastBindingStyle == nullptr)
	{
		return FLinearColor::White;
	}

	if (const UMDFastBindingDestinationBase* BindingDest = Cast<UMDFastBindingDestinationBase>(GetBindingObject()))
	{
		if (BindingDest->IsActive())
		{
			return FastBindingStyle->GetColor(TEXT("DestinationNodeTitleColor"));
		}

		return FastBindingStyle->GetColor(TEXT("InactiveDestinationNodeTitleColor"));
	}

	return FastBindingStyle->GetColor(TEXT("NodeTitleColor"));
}

FText UMDFastBindingGraphNode::GetTooltipText() const
{
	if (UMDFastBindingDestinationBase* BindingDest = Cast<UMDFastBindingDestinationBase>(GetBindingObject()))
	{
		if (!BindingDest->IsActive())
		{
			static const FText InactiveToolTipFormat = LOCTEXT("InactiveDestinationNodeToolTipFormat", "This binding destination is inactive and will not run. Right-click this node to set it active. Only one destination can be active within a binding at a time.\n{0}");
			return FText::Format(InactiveToolTipFormat, BindingDest->GetToolTipText());
		}
	}

	if (UMDFastBindingObject* Object = BindingObject.Get())
	{
		return Object->GetToolTipText();
	}

	return Super::GetTooltipText();
}

void UMDFastBindingGraphNode::RefreshGraph()
{
	if (UMDFastBindingGraph* MDGraph = Cast<UMDFastBindingGraph>(GetGraph()))
	{
		MDGraph->RefreshGraph();
	}
}

UBlueprint* UMDFastBindingGraphNode::GetBlueprint() const
{
	if (const UMDFastBindingGraph* MDGraph = Cast<UMDFastBindingGraph>(GetGraph()))
	{
		return MDGraph->GetBlueprint();
	}

	return nullptr;
}

void UMDFastBindingGraphNode::AllocateDefaultPins()
{
	if (UMDFastBindingObject* Object = BindingObject.Get())
	{
		Object->SetupBindingItems_Internal();

		UBlueprint* Blueprint = GetBlueprint();

		UFunction* Function = nullptr;
		if (UMDFastBindingDestination_Function* DestFunc = Cast<UMDFastBindingDestination_Function>(BindingObject))
		{
			Function = DestFunc->GetFunction();
		}
		else if (UMDFastBindingValue_Function* ValueFunc = Cast<UMDFastBindingValue_Function>(BindingObject))
		{
			Function = ValueFunc->GetFunction();
		}

		auto SetupPinTextData = [this, Function](UEdGraphPin* Pin, const FProperty* ItemProp, const FMDFastBindingItem* Item)
		{
			if (Pin == nullptr)
			{
				return;
			}

			if (Item != nullptr && !Item->DisplayName.IsEmptyOrWhitespace())
			{
				Pin->PinFriendlyName = Item->DisplayName;
			}
			else if (ItemProp != nullptr)
			{
				Pin->PinFriendlyName = ItemProp->HasMetaData(FBlueprintMetadata::MD_DisplayName)
					? FText::FromString(ItemProp->GetMetaData(FBlueprintMetadata::MD_DisplayName))
					: FText::GetEmpty();
			}

			if (Item != nullptr && !Item->ToolTip.IsEmptyOrWhitespace())
			{
				Pin->PinToolTip = Item->ToolTip.ToString();
			}
			else if (IsValid(Function))
			{
				// HACK - Temporarily swap our schema since UEdGraphSchema_K2::ConstructBasicPinTooltip _needlessly_ ensures if it's not the K2 Schema
				if (UMDFastBindingGraph* MDGraph = Cast<UMDFastBindingGraph>(GetGraph()))
				{
					TGuardValue<TSubclassOf<UEdGraphSchema>> SchemaGuard(MDGraph->Schema, UEdGraphSchema_K2::StaticClass());
					UK2Node_CallFunction::GeneratePinTooltipFromFunction(*Pin, Function);
				}
			}
			else if (ItemProp != nullptr)
			{
				static const FText PinToolTipFormat = LOCTEXT("PinToolTipFormat", "{0} ({1}) \n{2}");
				Pin->PinToolTip = FText::Format(PinToolTipFormat, ItemProp->GetDisplayNameText(), FText::FromString(FMDFastBindingHelpers::PropertyToString(*ItemProp)), ItemProp->GetToolTipText()).ToString();
			}
		};

#if UE_VERSION_OLDER_THAN(5, 4, 0)
		auto BackPortDeterministicGuid = [](FStringView ObjectPath, uint32 Seed)
		{
			// Convert the objectpath to utf8 so that whether TCHAR is UTF8 or UTF16 does not alter the hash.
#if UE_VERSION_OLDER_THAN(5, 3, 0)
			TUtf8StringBuilder<1024> Utf8ObjectPath;
			Utf8ObjectPath << ObjectPath;
#else
			TUtf8StringBuilder<1024> Utf8ObjectPath(InPlace, ObjectPath);
#endif

			FBlake3 Builder;

			// Hash this as the namespace of the Version 3 UUID, to avoid collisions with any other guids created using Blake3.
			static FGuid BaseVersion(TEXT("182b8dd3-f963-477f-a57d-70a339d922d8"));
			Builder.Update(&BaseVersion, sizeof(FGuid));
			Builder.Update(&Seed, sizeof(Seed));
			Builder.Update(Utf8ObjectPath.GetData(), Utf8ObjectPath.Len() * sizeof(UTF8CHAR));

			FBlake3Hash Hash = Builder.Finalize();

			// We use the first 16 bytes of the BLAKE3 hash to create the guid, there is no specific reason why these were
			// chosen, we could take any pattern or combination of bytes.
			uint32* HashBytes = (uint32*)Hash.GetBytes();
			uint32 A = uint32(HashBytes[0]);
			uint32 B = uint32(HashBytes[1]);
			uint32 C = uint32(HashBytes[2]);
			uint32 D = uint32(HashBytes[3]);

			// Convert to a Variant 1 Version 3 UUID, which is meant to be created by hashing the namespace and name with MD5.
			// We use that version even though we're using BLAKE3 instead of MD5 because we don't have a better alternative.
			// It is still very useful for avoiding collisions with other UUID variants.
			B = (B & ~0x0000f000) | 0x00003000; // Version 3 (MD5)
			C = (C & ~0xc0000000) | 0x80000000; // Variant 1 (RFC 4122 UUID)
			return FGuid(A, B, C, D);
		};
#endif

		for (FMDFastBindingItem& Item : Object->GetBindingItems())
		{
			UEdGraphPin* Pin = nullptr;
			const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
			const FProperty* ItemProp = Item.ResolveOutputProperty();
			if (ItemProp != nullptr)
			{
				FEdGraphPinType ItemPinType;
				K2Schema->ConvertPropertyToPinType(ItemProp, ItemPinType);
				Pin = CreatePin(EGPD_Input, ItemPinType, Item.ItemName);
			}
			else
			{
				Pin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, Item.ItemName);
			}

			if (Pin != nullptr)
			{
#if UE_VERSION_OLDER_THAN(5, 4, 0)
				Pin->PinId = BackPortDeterministicGuid(Item.ItemName.ToString(), GetTypeHash(Object->BindingObjectIdentifier));
#else
				Pin->PinId = FGuid::NewDeterministicGuid(Item.ItemName.ToString(), GetTypeHash(Object->BindingObjectIdentifier));
#endif

				if (Item.bIsWorldContextPin)
				{
					Pin->bHidden = !(Blueprint && Blueprint->ParentClass && Blueprint->ParentClass->HasMetaDataHierarchical(FBlueprintMetadata::MD_ShowWorldContextPin));
					Pin->bNotConnectable = Pin->bHidden;
				}

				if (Item.HasValue())
				{
					Pin->DefaultObject = Item.DefaultObject;
					Pin->DefaultValue = Item.DefaultString;
					Pin->DefaultTextValue = Item.DefaultText;
				}
				else
				{
					// Special cases for color structs
					UScriptStruct* LinearColorStruct = TBaseStructure<FLinearColor>::Get();
					UScriptStruct* ColorStruct = TBaseStructure<FColor>::Get();

					FString ParamValue;
					if (Function != nullptr && K2Schema->FindFunctionParameterDefaultValue(Function, ItemProp, ParamValue))
					{
						K2Schema->SetPinAutogeneratedDefaultValue(Pin, ParamValue);
					}
					else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct && Pin->PinType.PinSubCategoryObject == ColorStruct)
					{
						K2Schema->SetPinAutogeneratedDefaultValue(Pin, FColor::White.ToString());
					}
					else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct && Pin->PinType.PinSubCategoryObject == LinearColorStruct)
					{
						K2Schema->SetPinAutogeneratedDefaultValue(Pin, FLinearColor::White.ToString());
					}
					else
					{
						K2Schema->SetPinAutogeneratedDefaultValueBasedOnType(Pin);
					}

					const bool bDidDefaultsChange = Item.DefaultObject != Pin->DefaultObject
						|| Item.DefaultString != Pin->DefaultValue
						|| !Item.DefaultText.EqualTo(Pin->DefaultTextValue);
					if (bDidDefaultsChange)
					{
						Object->Modify();
						Item.DefaultObject = Pin->DefaultObject;
						Item.DefaultString = Pin->DefaultValue;
						Item.DefaultText = Pin->DefaultTextValue;
					}
				}
			}

			SetupPinTextData(Pin, ItemProp, &Item);
		}

		if (UMDFastBindingValueBase* ValueObject = Cast<UMDFastBindingValueBase>(Object))
		{
			if (const FProperty* OutputProp = ValueObject->GetOutputProperty())
			{
				FEdGraphPinType OutputPinType;
				GetDefault<UEdGraphSchema_K2>()->ConvertPropertyToPinType(OutputProp, OutputPinType);
				UEdGraphPin* Pin = CreatePin(EGPD_Output, OutputPinType, OutputPinName);
#if UE_VERSION_OLDER_THAN(5, 4, 0)
				Pin->PinId = BackPortDeterministicGuid(OutputPinName.ToString(), GetTypeHash(Object->BindingObjectIdentifier));
#else
				Pin->PinId = FGuid::NewDeterministicGuid(OutputPinName.ToString(), GetTypeHash(Object->BindingObjectIdentifier));
#endif
				SetupPinTextData(Pin, OutputProp, nullptr);
			}
			else
			{
				// Create an invalid pin so we show _something_
				UEdGraphPin* Pin = CreatePin(EGPD_Output, NAME_None, OutputPinName);
#if UE_VERSION_OLDER_THAN(5, 4, 0)
				Pin->PinId = BackPortDeterministicGuid(OutputPinName.ToString(), GetTypeHash(Object->BindingObjectIdentifier));
#else
				Pin->PinId = FGuid::NewDeterministicGuid(OutputPinName.ToString(), GetTypeHash(Object->BindingObjectIdentifier));
#endif
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
