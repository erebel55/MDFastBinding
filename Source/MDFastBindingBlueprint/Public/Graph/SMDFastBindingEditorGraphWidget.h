﻿// Copyright Dylan Dumesnil. All Rights Reserved.

#pragma once

#include "GraphEditor.h"
#include "Widgets/SCompoundWidget.h"

class UMDFastBindingGraph;
class UMDFastBindingInstance;
class UMDFastBindingObject;
class UMDFastBindingValueBase;
class UMDFastBindingDestinationBase;
struct FEdGraphSchemaAction;
struct FGraphActionListBuilderBase;

/**
 *
 */
class MDFASTBINDINGBLUEPRINT_API SMDFastBindingEditorGraphWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMDFastBindingEditorGraphWidget)
		{
		}

		SLATE_EVENT(SGraphEditor::FOnSelectionChanged, OnSelectionChanged)
	SLATE_END_ARGS()

	virtual ~SMDFastBindingEditorGraphWidget() override;

	void Construct(const FArguments& InArgs, UBlueprint* Blueprint);

	void SetBinding(UMDFastBindingInstance* InBinding);
	void SetBindingBeingDebugged(UMDFastBindingInstance* InBinding);

	void RefreshGraph() const;

	void SetSelection(UEdGraphNode* InNode, bool bIsSelected);
	void ClearSelection();

private:
	void OnNodeTitleChanged(const FText& InText, ETextCommit::Type Type, UEdGraphNode* Node);

	void RegisterCommands();

	const TArray<TSubclassOf<UMDFastBindingValueBase>>& GetValueClasses();
	const TArray<TSubclassOf<UMDFastBindingDestinationBase>>& GetDestinationClasses();

	FActionMenuContent OnCreateNodeOrPinMenu(UEdGraph* CurrentGraph, const UEdGraphNode* InGraphNode, const UEdGraphPin* InGraphPin, FMenuBuilder* MenuBuilder, bool bIsDebugging);

	bool CanDeleteSelectedNodes() const;
	void DeleteSelectedNodes() const;

	void RenameSelectedNode() const;

	void CopySelectedNodes() const;

	void PasteNodes() const;
	bool CanPasteNodes() const;

	void CutSelectedNodes() const;
	bool CanCutSelectedNodes() const;

	bool CanSetDestinationActive() const;
	void SetDestinationActive() const;

	void RemoveExtendablePin(TWeakObjectPtr<UMDFastBindingObject> BindingObject, int32 ItemIndex) const;

	TSharedPtr<SGraphEditor> GraphEditor;

	UMDFastBindingGraph* GraphObj = nullptr;

	TWeakObjectPtr<UMDFastBindingInstance> Binding;
	TWeakObjectPtr<UMDFastBindingInstance> BindingBeingDebugged;

	TArray<TSubclassOf<UMDFastBindingValueBase>> ValueClasses;

	TArray<TSubclassOf<UMDFastBindingDestinationBase>> DestinationClasses;

	TSharedPtr<FUICommandList> GraphEditorCommands;
};
