#pragma once

#include "EdGraph/EdGraph.h"
#include "Runtime/Launch/Resources/Version.h"
#include "MDFastBindingGraph.generated.h"

class UMDFastBindingObject;
class UMDFastBindingInstance;
class UMDFastBindingGraphNode;
class SMDFastBindingEditorGraphWidget;

/**
 *
 */
UCLASS()
class MDFASTBINDINGBLUEPRINT_API UMDFastBindingGraph : public UEdGraph
{
	GENERATED_BODY()

public:
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 3
	virtual void AddNode(UEdGraphNode* NodeToAdd, bool bUserAction, bool bSelectNewNode) override;
#endif

	void SetGraphWidget(TSharedRef<SMDFastBindingEditorGraphWidget> InGraphWidget);

	void RefreshGraph();

	void SetBinding(UMDFastBindingInstance* InBinding);
	UMDFastBindingInstance* GetBinding() const { return Binding.Get(); }

	void SetBindingBeingDebugged(UMDFastBindingInstance* InBinding);
	UMDFastBindingInstance* GetBindingBeingDebugged() const { return BindingBeingDebugged.Get(); }

	UMDFastBindingGraphNode* FindNodeWithBindingObject(UMDFastBindingObject* InObject) const;
	void SelectNodeWithBindingObject(UMDFastBindingObject* InObject);

	void ClearSelection();

	UBlueprint* GetBlueprint() const;

private:
	TWeakObjectPtr<UMDFastBindingInstance> Binding;
	TWeakObjectPtr<UMDFastBindingInstance> BindingBeingDebugged;

	TWeakPtr<SMDFastBindingEditorGraphWidget> GraphWidget;
};
