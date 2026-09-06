// Fill out your copyright notice in the Description page of Project Settings.


#include "UIBaseView.h"

#include "MVVMSubsystem.h"
#include "UIBaseLog.h"
#include "UIBaseViewModel.h"
#include "View/MVVMView.h"

void
UUIBaseView::SetViewModel( UUIBaseViewModel * InViewModel )
{
	ViewModel = InViewModel;
	
	if ( UMVVMView * MvvmView = UMVVMSubsystem::GetViewFromUserWidget( this ) )
	{
		if ( !MvvmView->SetViewModelByClass( InViewModel ) )
		{
			UE_LOG( LogUIBase, Warning,
				TEXT("%s: SetViewModelByClass failed for %s."),
				*GetName(),
				*GetNameSafe( InViewModel ? InViewModel->GetClass() : nullptr )
				);
		}
	}
	
	OnViewModelSet();
}

void
UUIBaseView::NativeDestruct()
{
	ViewModel = nullptr;
	Super::NativeDestruct();
}