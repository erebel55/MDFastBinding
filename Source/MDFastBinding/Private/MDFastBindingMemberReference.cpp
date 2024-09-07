﻿// Copyright Dylan Dumesnil. All Rights Reserved.

#include "MDFastBindingMemberReference.h"

void FMDFastBindingMemberReference::FixUpReference(UClass& OwnerClass) const
{
	if (OwnerClass.HasAnyClassFlags(CLASS_NewerVersionExists) || (OwnerClass.HasAnyFlags(RF_Transient) && OwnerClass.HasAnyClassFlags(CLASS_CompiledFromBlueprint)))
	{
		return;
	}

	const bool bIsValid = bIsFunction
		? IsValid(ResolveMember<UFunction>(&OwnerClass, true))
		: (ResolveMember<FProperty>(&OwnerClass, true) != nullptr);

#if WITH_EDITOR
	UClass* EffectiveClass = OwnerClass.GetAuthoritativeClass();
#else
	UClass* EffectiveClass = &OwnerClass;
#endif
	if (!bIsValid || !EffectiveClass->IsChildOf(GetMemberParentClass()))
	{
		MemberParent = EffectiveClass;
	}
}
