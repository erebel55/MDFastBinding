﻿#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UMDFastBindingGraph;
class UMDFastBindingDestinationBase;
class UMDFastBindingObject;
class UMDFastBindingValueBase;
class SGraphEditor;

/**
 * 
 */
class MDFASTBINDINGEDITOR_API SMDFastBindingEditorGraphWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMDFastBindingEditorGraphWidget)
		{
		}

		SLATE_EVENT(SGraphEditor::FOnSelectionChanged, OnSelectionChanged)
	SLATE_END_ARGS()

	virtual ~SMDFastBindingEditorGraphWidget() override;

	void Construct(const FArguments& InArgs);

	void SetBinding(UMDFastBindingDestinationBase* InBinding);

	void RefreshGraph() const;

	void SetSelection(UEdGraphNode* InNode, bool bIsSelected);
	void ClearSelection();

private:
	void OnNodeTitleChanged(const FText& InText, ETextCommit::Type Type, UEdGraphNode* Node);

	void RegisterCommands();
	
	const TArray<TSubclassOf<UMDFastBindingValueBase>>& GetValueClasses();

	FActionMenuContent OnCreateActionMenu(UEdGraph* InGraph, const FVector2D& InNodePosition, const TArray<UEdGraphPin*>& InDraggedPins, bool bAutoExpand, SGraphEditor::FActionMenuClosed InOnMenuClose);
	FActionMenuContent OnCreateNodeOrPinMenu(UEdGraph* CurrentGraph, const UEdGraphNode* InGraphNode, const UEdGraphPin* InGraphPin, FMenuBuilder* MenuBuilder, bool bIsDebugging);
	
	void OnActionSelected(const TArray<TSharedPtr<FEdGraphSchemaAction>>& SelectedActions, ESelectInfo::Type InSelectionType, FVector2D InNodePosition, TArray<UEdGraphPin*> InDraggedPins) const;
	void CollectAllActions(FGraphActionListBuilderBase& OutAllActions, TArray<UEdGraphPin*> InDraggedPins);

	bool CanDeleteSelectedNodes() const;
	void DeleteSelectedNodes() const;
	
	void RenameSelectedNode() const;
	
	TSharedPtr<SGraphEditor> GraphEditor;

	UMDFastBindingGraph* GraphObj = nullptr;

	TWeakObjectPtr<UMDFastBindingDestinationBase> Binding;

	TArray<TSubclassOf<UMDFastBindingValueBase>> ValueClasses;
	
	TSharedPtr<FUICommandList> GraphEditorCommands;
};
