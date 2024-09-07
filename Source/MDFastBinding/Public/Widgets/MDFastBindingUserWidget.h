﻿// Copyright Dylan Dumesnil. All Rights Reserved.

#pragma once

#include "MDFastBindingContainer.h"
#include "Blueprint/UserWidget.h"
#include "MDFastBindingUserWidget.generated.h"

class UMDFastBindingContainer;
class UE_DEPRECATED(5.1, "UMDFastBindingUserWidget has been deprecated, bindings now work via Widget Extensions. Use UUserWidget or your own base class instead.") UMDFastBindingUserWidget;

/**
 * Base user widget class that has built-in Fast Binding functionality
 */
UCLASS(Abstract)
class MDFASTBINDING_API UMDFastBindingUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	UPROPERTY(Instanced, DuplicateTransient)
	UMDFastBindingContainer* Bindings = nullptr;

};
