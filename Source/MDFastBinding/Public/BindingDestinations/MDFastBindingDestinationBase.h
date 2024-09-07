// Copyright Dylan Dumesnil. All Rights Reserved.

#pragma once

#include "MDFastBindingObject.h"
#include "MDFastBindingDestinationBase.generated.h"

class UMDFastBindingValueBase;

/**
 *
 */
UCLASS(Abstract, DefaultToInstanced, EditInlineNew)
class MDFASTBINDING_API UMDFastBindingDestinationBase : public UMDFastBindingObject
{
	GENERATED_BODY()

public:
	void InitializeDestination(UObject* SourceObject);
	void UpdateDestination(UObject* SourceObject);
	void TerminateDestination(UObject* SourceObject);

	virtual bool HasRunSuccessfully() const override { return HasEverUpdated(); }

#if WITH_EDITOR
	bool IsActive() const;
#endif

protected:
	virtual void InitializeDestination_Internal(UObject* SourceObject) {}
	virtual void UpdateDestination_Internal(UObject* SourceObject) {}
	virtual void TerminateDestination_Internal(UObject* SourceObject) {}

	// Must be called manually by child classes after updated the destination
	void MarkAsHasEverUpdated();

	bool HasEverUpdated() const { return bHasEverUpdated; }

private:
	UPROPERTY(Transient)
	bool bHasEverUpdated = false;
};
