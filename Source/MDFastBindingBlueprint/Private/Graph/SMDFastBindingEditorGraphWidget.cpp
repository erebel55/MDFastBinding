﻿// Copyright Dylan Dumesnil. All Rights Reserved.

#include "Graph/SMDFastBindingEditorGraphWidget.h"

#include "Algo/Transform.h"
#include "BindingDestinations/MDFastBindingDestinationBase.h"
#include "BindingValues/MDFastBindingValueBase.h"
#include "Debug/MDFastBindingDebugPersistentData.h"
#include "EdGraphUtilities.h"
#include "Engine/Blueprint.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Graph/MDFastBindingEditorCommands.h"
#include "Graph/MDFastBindingGraph.h"
#include "Graph/MDFastBindingGraphNode.h"
#include "Graph/MDFastBindingGraphSchema.h"
#include "MDFastBindingInstance.h"
#include "MDFastBindingObject.h"
#include "SGraphActionMenu.h"
#include "ScopedTransaction.h"
#include "SequencerSettings.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/Input/SEditableTextBox.h"

#define LOCTEXT_NAMESPACE "SMDFastBindingEditorGraphWidget"

SMDFastBindingEditorGraphWidget::~SMDFastBindingEditorGraphWidget()
{
	GraphObj->RemoveFromRoot();
}

void SMDFastBindingEditorGraphWidget::Construct(const FArguments& InArgs, UBlueprint* Blueprint)
{
	GraphObj = NewObject<UMDFastBindingGraph>(Blueprint);
	GraphObj->AddToRoot();
	GraphObj->Schema = UMDFastBindingGraphSchema::StaticClass();
	GraphObj->SetGraphWidget(SharedThis(this));

	SGraphEditor::FGraphEditorEvents GraphEvents;
	GraphEvents.OnTextCommitted.BindSP(this, &SMDFastBindingEditorGraphWidget::OnNodeTitleChanged);
	GraphEvents.OnSelectionChanged = InArgs._OnSelectionChanged;
	GraphEvents.OnCreateNodeOrPinMenu.BindSP(this, &SMDFastBindingEditorGraphWidget::OnCreateNodeOrPinMenu);

	RegisterCommands();

	GraphEditor = SNew(SGraphEditor)
		.GraphToEdit(GraphObj)
		.AdditionalCommands(GraphEditorCommands)
		.GraphEvents(GraphEvents);

	ChildSlot
	[
		GraphEditor.ToSharedRef()
	];
}

void SMDFastBindingEditorGraphWidget::RegisterCommands()
{
	if (GraphEditorCommands.IsValid())
	{
		return;
	}

	FMDFastBindingEditorCommands::Register();

	GraphEditorCommands = MakeShared<FUICommandList>();

	GraphEditorCommands->MapAction(FGenericCommands::Get().Delete,
		FExecuteAction::CreateSP(this, &SMDFastBindingEditorGraphWidget::DeleteSelectedNodes),
		FCanExecuteAction::CreateSP(this, &SMDFastBindingEditorGraphWidget::CanDeleteSelectedNodes)
		);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Rename,
		FExecuteAction::CreateSP(this, &SMDFastBindingEditorGraphWidget::RenameSelectedNode),
		FCanExecuteAction()
		);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Copy,
		FExecuteAction::CreateSP(this, &SMDFastBindingEditorGraphWidget::CopySelectedNodes),
		FCanExecuteAction()
		);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Paste,
		FExecuteAction::CreateSP(this, &SMDFastBindingEditorGraphWidget::PasteNodes),
		FCanExecuteAction::CreateSP(this, &SMDFastBindingEditorGraphWidget::CanPasteNodes)
		);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Cut,
		FExecuteAction::CreateSP(this, &SMDFastBindingEditorGraphWidget::CutSelectedNodes),
		FCanExecuteAction::CreateSP(this, &SMDFastBindingEditorGraphWidget::CanCutSelectedNodes)
		);

	GraphEditorCommands->MapAction(FMDFastBindingEditorCommands::Get().SetDestinationActive,
		FExecuteAction::CreateSP(this, &SMDFastBindingEditorGraphWidget::SetDestinationActive),
		FCanExecuteAction::CreateSP(this, &SMDFastBindingEditorGraphWidget::CanSetDestinationActive)
		);
}

void SMDFastBindingEditorGraphWidget::SetBinding(UMDFastBindingInstance* InBinding)
{
	Binding = InBinding;
	GraphObj->SetBinding(InBinding);
}

void SMDFastBindingEditorGraphWidget::SetBindingBeingDebugged(UMDFastBindingInstance* InBinding)
{
	BindingBeingDebugged = InBinding;
	GraphObj->SetBindingBeingDebugged(InBinding);
}

void SMDFastBindingEditorGraphWidget::RefreshGraph() const
{
	GraphObj->RefreshGraph();
}

void SMDFastBindingEditorGraphWidget::SetSelection(UEdGraphNode* InNode, bool bIsSelected)
{
	GraphEditor->SetNodeSelection(InNode, bIsSelected);
}

void SMDFastBindingEditorGraphWidget::ClearSelection()
{
	GraphEditor->ClearSelectionSet();
}

void SMDFastBindingEditorGraphWidget::OnNodeTitleChanged(const FText& InText, ETextCommit::Type Type,
                                                         UEdGraphNode* Node)
{
	if (Node != nullptr)
	{
		Node->OnRenameNode(InText.ToString());
	}
}

const TArray<TSubclassOf<UMDFastBindingValueBase>>& SMDFastBindingEditorGraphWidget::GetValueClasses()
{
	if (ValueClasses.Num() == 0)
	{
		for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
		{
			if (ClassIt->IsChildOf(UMDFastBindingValueBase::StaticClass()) && !ClassIt->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_Hidden))
			{
				ValueClasses.Add(*ClassIt);
			}
		}

		ValueClasses.Sort([](const TSubclassOf<UMDFastBindingValueBase>& A, const TSubclassOf<UMDFastBindingValueBase>& B)
		{
			return A->GetDisplayNameText().CompareTo(B->GetDisplayNameText()) < 0;
		});
	}

	return ValueClasses;
}

const TArray<TSubclassOf<UMDFastBindingDestinationBase>>& SMDFastBindingEditorGraphWidget::GetDestinationClasses()
{
	if (DestinationClasses.Num() == 0)
	{
		for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
		{
			if (ClassIt->IsChildOf(UMDFastBindingDestinationBase::StaticClass()) && !ClassIt->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_Hidden))
			{
				DestinationClasses.Add(*ClassIt);
			}
		}

		DestinationClasses.Sort([](const TSubclassOf<UMDFastBindingDestinationBase>& A, const TSubclassOf<UMDFastBindingDestinationBase>& B)
		{
			return A->GetDisplayNameText().CompareTo(B->GetDisplayNameText()) < 0;
		});
	}

	return DestinationClasses;
}

FActionMenuContent SMDFastBindingEditorGraphWidget::OnCreateNodeOrPinMenu(UEdGraph* CurrentGraph, const UEdGraphNode* InGraphNode, const UEdGraphPin* InGraphPin, FMenuBuilder* MenuBuilder, bool bIsDebugging)
{
	// Menu when right clicking

	if (const UMDFastBindingGraphNode* GraphNode = Cast<UMDFastBindingGraphNode>(InGraphNode))
	{
		MenuBuilder->AddMenuEntry(FGenericCommands::Get().Rename);
		MenuBuilder->AddMenuEntry(FGenericCommands::Get().Delete);

		if (UMDFastBindingObject* BindingObject = GraphNode->GetBindingObject())
		{
			if (const UMDFastBindingDestinationBase* BindingDest = Cast<UMDFastBindingDestinationBase>(BindingObject))
			{
				if (!BindingDest->IsActive())
				{
					MenuBuilder->AddMenuEntry(FMDFastBindingEditorCommands::Get().SetDestinationActive);
				}
			}

			if (InGraphPin != nullptr)
			{
				if (const FMDFastBindingItem* PinItem = BindingObject->FindBindingItem(InGraphPin->GetFName()))
				{
					if (PinItem->ExtendablePinListIndex != INDEX_NONE)
					{
						MenuBuilder->AddMenuEntry(
							LOCTEXT("RemovePin", "Remove pin"),
							LOCTEXT("RemovePinTooltip", "Remove this input pin (and any accompanying pins with the same index)"),
							FSlateIcon(),
							FUIAction(
								FExecuteAction::CreateSP(this, &SMDFastBindingEditorGraphWidget::RemoveExtendablePin, MakeWeakObjectPtr(BindingObject), PinItem->ExtendablePinListIndex)
							)
						);
					}
				}

				// Pin debugging actions
				{
					// Outputs don't have values to watch, so instead watch the pin we're linked to
					const UEdGraphPin* EffectivePin = [&]() -> const UEdGraphPin*
					{
						if (InGraphPin->Direction == EGPD_Input)
						{
							return InGraphPin;
						}

						if (InGraphPin->LinkedTo.Num() > 0)
						{
							return InGraphPin->LinkedTo[0];
						}

						return nullptr;
					}();

					const UEdGraphNode* EffectiveNode = EffectivePin != nullptr ? EffectivePin->GetOwningNode() : nullptr;
					if (const UMDFastBindingGraphNode* Node = Cast<UMDFastBindingGraphNode>(EffectiveNode))
					{
						if (const UMDFastBindingObject* EffectiveBindingObject = Node->GetBindingObject())
						{
							if (UMDFastBindingDebugPersistentData::Get().IsPinBeingWatched(EffectiveBindingObject->BindingObjectIdentifier, EffectivePin->PinName))
							{
								MenuBuilder->AddMenuEntry(
									FText::Format(LOCTEXT("MDFastBindingEditor_RemovePinWatch", "Remove '{0}' from Watch List"), FText::FromName(EffectivePin->PinName)),
									LOCTEXT("MDFastBindingEditor_RemovePinWatchTooltip", "Remove this pin from the watch window"),
									FSlateIcon(),
									FUIAction(
										FExecuteAction::CreateLambda([NodeID = EffectiveBindingObject->BindingObjectIdentifier, PinName = EffectivePin->PinName]()
										{
											UMDFastBindingDebugPersistentData::Get().RemovePinFromWatchList(NodeID, PinName);
										}),
										FCanExecuteAction()
									)
								);
							}
							else
							{
								MenuBuilder->AddMenuEntry(
									FText::Format(LOCTEXT("MDFastBindingEditor_AddPinWatch", "Add '{0}' to Watch List"), FText::FromName(EffectivePin->PinName)),
									LOCTEXT("MDFastBindingEditor_AddPinWatchTooltip", "Add this pin from the watch window"),
									FSlateIcon(),
									FUIAction(
									FExecuteAction::CreateLambda([NodeID = EffectiveBindingObject->BindingObjectIdentifier, PinName = EffectivePin->PinName]()
										{
											UMDFastBindingDebugPersistentData::Get().AddPinToWatchList(NodeID, PinName);
										}),
										FCanExecuteAction()
									)
								);
							}
						}
					}
				}
			}
		}
	}

	return FActionMenuContent(MenuBuilder->MakeWidget());
}

bool SMDFastBindingEditorGraphWidget::CanDeleteSelectedNodes() const
{
	const FGraphPanelSelectionSet& SelectedNodes = GraphEditor->GetSelectedNodes();
	for (UObject* SelectedNode : SelectedNodes)
	{
		if (SelectedNode != nullptr && SelectedNode->IsA<UMDFastBindingGraphNode>())
		{
			return true;
		}
	}

	return false;
}

void SMDFastBindingEditorGraphWidget::DeleteSelectedNodes() const
{
	const FScopedTransaction Transaction(FGenericCommands::Get().Delete->GetDescription());

	const TSet<UObject*>& SelectedNodeSet = GraphEditor->GetSelectedNodes();

	TSet<UObject*> ValuesBeingDeleted;
	Algo::Transform(SelectedNodeSet, ValuesBeingDeleted, [](UObject* InNode) -> UMDFastBindingObject*
	{
		if (const UMDFastBindingGraphNode* GraphNode = Cast<UMDFastBindingGraphNode>(InNode))
		{
			return GraphNode->GetBindingObject();
		}

		return nullptr;
	});

	for (UObject* SelectedNode : SelectedNodeSet)
	{
		if (UMDFastBindingGraphNode* GraphNode = Cast<UMDFastBindingGraphNode>(SelectedNode))
		{
			GraphNode->DeleteNode(ValuesBeingDeleted);
		}
	}

	GraphEditor->ClearSelectionSet();
	RefreshGraph();
}

void SMDFastBindingEditorGraphWidget::RenameSelectedNode() const
{
	const FGraphPanelSelectionSet& SelectedNodes = GraphEditor->GetSelectedNodes();
	for (UObject* SelectedNode : SelectedNodes)
	{
		if (const UMDFastBindingGraphNode* GraphNode = Cast<UMDFastBindingGraphNode>(SelectedNode))
		{
			GraphEditor->JumpToNode(GraphNode, true);
			break;
		}
	}
}

void SMDFastBindingEditorGraphWidget::CopySelectedNodes() const
{
	const FGraphPanelSelectionSet& SelectedNodes = GraphEditor->GetSelectedNodes();

	// Populate a list of selected binding objects for copying
	TSet<FGuid> SelectedObjects;
	for (UObject* NodeObject : SelectedNodes)
	{
		if (const UMDFastBindingGraphNode* BindingNode = Cast<UMDFastBindingGraphNode>(NodeObject))
		{
			SelectedObjects.Add(BindingNode->GetBindingObject()->BindingObjectIdentifier);
		}
	}

	// We only need the nodes that aren't owned by a selected node
	TSet<UObject*> NodesToCopy;
	for (UObject* NodeObject : SelectedNodes)
	{
		if (UMDFastBindingGraphNode* BindingNode = Cast<UMDFastBindingGraphNode>(NodeObject))
		{
			const UMDFastBindingGraphNode* LinkedNode = BindingNode->GetLinkedOutputNode();
			if (LinkedNode == nullptr || !SelectedObjects.Contains(LinkedNode->GetBindingObject()->BindingObjectIdentifier))
			{
				BindingNode->PrepareForCopying();
				NodesToCopy.Add(BindingNode);
			}
		}
	}

	// We need to clear any references to nodes that aren't selected since we don't want them copied
	for (UObject* NodeObject : NodesToCopy)
	{
		if (const UMDFastBindingGraphNode* BindingNode = Cast<UMDFastBindingGraphNode>(NodeObject))
		{
			if (UMDFastBindingObject* BindingObject = BindingNode->GetCopiedBindingObject())
			{
				for (FMDFastBindingItem& BindingItem : BindingObject->GetBindingItems())
				{
					if (BindingItem.Value != nullptr && !SelectedObjects.Contains(BindingItem.Value->BindingObjectIdentifier))
					{
						BindingItem.Value = nullptr;
					}
				}
			}
		}
	}

	if (NodesToCopy.Num() > 0)
	{
		FString ExportedText;
		FEdGraphUtilities::ExportNodesToText(NodesToCopy, /*out*/ ExportedText);
		FPlatformApplicationMisc::ClipboardCopy(*ExportedText);
	}

	for (UObject* NodeObject : NodesToCopy)
	{
		if (UMDFastBindingGraphNode* BindingNode = Cast<UMDFastBindingGraphNode>(NodeObject))
		{
			BindingNode->CleanUpCopying();
		}
	}
}

void SMDFastBindingEditorGraphWidget::PasteNodes() const
{
	GraphEditor->ClearSelectionSet();

	// Grab the text to paste from the clipboard.
	FString TextToImport;
	FPlatformApplicationMisc::ClipboardPaste(TextToImport);

	// Create temporary graph
	const FName UniqueGraphName = MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), FName(*(LOCTEXT("MDFastBindingTempGraph", "TempGraph").ToString())));
	const TStrongObjectPtr<UMDFastBindingGraph> TempBindingGraph = TStrongObjectPtr<UMDFastBindingGraph>(NewObject<UMDFastBindingGraph>(GetTransientPackage(), UniqueGraphName));
	TempBindingGraph->Schema = UMDFastBindingGraphSchema::StaticClass();

	TSet<UEdGraphNode*> PastedNodes;
	FEdGraphUtilities::ImportNodesFromText(TempBindingGraph.Get(), TextToImport, /*out*/ PastedNodes);

	TArray<UMDFastBindingObject*> BindingObjects;
	TArray<UMDFastBindingObject*> OrphanObjects;
	for(UEdGraphNode* Node : PastedNodes)
	{
		UMDFastBindingGraphNode* BindingNode = Cast<UMDFastBindingGraphNode>(Node);
		if (BindingNode && BindingNode->CanDuplicateNode())
		{
			if (UMDFastBindingObject* CopiedObject = BindingNode->GetCopiedBindingObject())
			{
				BindingObjects.Add(CopiedObject);
				OrphanObjects.Add(CopiedObject);

				TArray<UMDFastBindingValueBase*> BindingValues;
				CopiedObject->GatherBindingValues(BindingValues);
				BindingObjects.Append(BindingValues);

				BindingNode->CleanUpPasting();
			}
		}
	}

	TOptional<FIntPoint> TopLeftMostNodePosition;
	for (const UMDFastBindingObject* Object : BindingObjects)
	{
		if (!TopLeftMostNodePosition.IsSet())
		{
			TopLeftMostNodePosition = Object->NodePos;
		}
		else if (Object->NodePos.X < TopLeftMostNodePosition.GetValue().X)
		{
			TopLeftMostNodePosition.GetValue().X = Object->NodePos.X;
		}
		else if (Object->NodePos.Y < TopLeftMostNodePosition.GetValue().Y)
		{
			TopLeftMostNodePosition.GetValue().Y = Object->NodePos.Y;
		}
	}

	FIntPoint AvgNodePosition = FIntPoint::ZeroValue;
	for (UMDFastBindingObject* Object : BindingObjects)
	{
		Object->NodePos -= TopLeftMostNodePosition.GetValue();
		AvgNodePosition += Object->NodePos;
	}

	if (BindingObjects.Num() > 0)
	{
		AvgNodePosition /= BindingObjects.Num();
	}

	const FIntPoint PasteLocation = FIntPoint(GraphEditor->GetPasteLocation().X, GraphEditor->GetPasteLocation().Y) - AvgNodePosition;
	for (UMDFastBindingObject* Object : BindingObjects)
	{
		Object->NodePos += PasteLocation;
	}

	// Re-roll all the guids for the new objects
	for (UMDFastBindingObject* BindingObject : BindingObjects)
	{
		if (BindingObject != nullptr)
		{
			BindingObject->BindingObjectIdentifier = FGuid::NewGuid();
		}
	}

	TArray<UMDFastBindingObject*> NewObjects;
	if(OrphanObjects.Num() > 0)
	{
		FScopedTransaction Transaction(FGenericCommands::Get().Paste->GetDescription());

		if(UMDFastBindingInstance* BindingPtr = Binding.Get())
		{
			for (UMDFastBindingObject* BindingObject : OrphanObjects)
			{
				if (UMDFastBindingDestinationBase* BindingDest = Cast<UMDFastBindingDestinationBase>(BindingObject))
				{
					if (UMDFastBindingDestinationBase* NewDest = BindingPtr->SetDestination(BindingDest))
					{
						NewObjects.Add(NewDest);

						TArray<UMDFastBindingValueBase*> BindingValues;
						NewDest->GatherBindingValues(BindingValues);
						NewObjects.Append(BindingValues);
					}
				}
				else if (UMDFastBindingValueBase* BindingValue = Cast<UMDFastBindingValueBase>(BindingObject))
				{
					if (UMDFastBindingValueBase* NewValue = BindingPtr->AddOrphan(BindingValue))
					{
						NewObjects.Add(NewValue);

						TArray<UMDFastBindingValueBase*> BindingValues;
						NewValue->GatherBindingValues(BindingValues);
						NewObjects.Append(BindingValues);
					}
				}
			}
		}
	}

	RefreshGraph();

	for (UMDFastBindingObject* Object : NewObjects)
	{
		if (UMDFastBindingGraphNode* Node = GraphObj->FindNodeWithBindingObject(Object))
		{
			GraphEditor->SetNodeSelection(Node, true);
		}
	}
}

bool SMDFastBindingEditorGraphWidget::CanPasteNodes() const
{
	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

	return FEdGraphUtilities::CanImportNodesFromText(GraphObj, ClipboardContent);
}

void SMDFastBindingEditorGraphWidget::CutSelectedNodes() const
{
	CopySelectedNodes();
	DeleteSelectedNodes();
}

bool SMDFastBindingEditorGraphWidget::CanCutSelectedNodes() const
{
	return CanDeleteSelectedNodes();
}

bool SMDFastBindingEditorGraphWidget::CanSetDestinationActive() const
{
	const FGraphPanelSelectionSet& SelectedNodes = GraphEditor->GetSelectedNodes();
	if (SelectedNodes.Num() != 1)
	{
		return false;
	}

	if (const UMDFastBindingGraphNode* GraphNode = Cast<UMDFastBindingGraphNode>(SelectedNodes.Array()[0]))
	{
		if (const UMDFastBindingDestinationBase* BindingDest = Cast<UMDFastBindingDestinationBase>(GraphNode->GetBindingObject()))
		{
			return !BindingDest->IsActive();
		}
	}

	return false;
}

void SMDFastBindingEditorGraphWidget::SetDestinationActive() const
{
	const FGraphPanelSelectionSet& SelectedNodes = GraphEditor->GetSelectedNodes();
	if (SelectedNodes.Num() != 1)
	{
		return;
	}

	if (const UMDFastBindingGraphNode* GraphNode = Cast<UMDFastBindingGraphNode>(SelectedNodes.Array()[0]))
	{
		if (UMDFastBindingDestinationBase* BindingDest = Cast<UMDFastBindingDestinationBase>(GraphNode->GetBindingObject()))
		{
			if (UMDFastBindingInstance* BindingPtr = Binding.Get())
			{
				BindingPtr->SetDestination(BindingDest);
				RefreshGraph();
			}
		}
	}
}

void SMDFastBindingEditorGraphWidget::RemoveExtendablePin(TWeakObjectPtr<UMDFastBindingObject> BindingObject, int32 ItemIndex) const
{
	if (UMDFastBindingObject* Object = BindingObject.Get())
	{
		FScopedTransaction Transaction(LOCTEXT("DeletePinTransactionName", "Delete Pin"));
		Object->Modify();
		Object->RemoveExtendablePinBindingItem(ItemIndex);
		RefreshGraph();
	}
}

#undef LOCTEXT_NAMESPACE
