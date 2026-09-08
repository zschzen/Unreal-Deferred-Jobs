// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"
#include "Tests/UIBaseTestTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIBaseControllerViewModelCacheTest,
	"UIBase.Controller.ViewModelCache",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIBaseControllerViewModelCacheTest::RunTest(const FString& Parameters)
{
	// GetTransientPackage() is the engine's throwaway package: the conventional
	// Outer for objects that exist only for the duration of a test
	UUIBaseTestController* Controller =
		NewObject<UUIBaseTestController>(GetTransientPackage(), UUIBaseTestController::StaticClass());
	Controller->Initialize(nullptr);

	UUIBaseViewModel* First = Controller->GetOrCreateViewModel(UUIBaseTestViewModel::StaticClass());
	TestNotNull(TEXT("first creation returns an instance"), First);
	TestTrue(TEXT("created viewmodel is already initialized"), First && First->IsInitialized());

	UUIBaseViewModel* Second = Controller->GetOrCreateViewModel(UUIBaseTestViewModel::StaticClass());
	TestEqual(TEXT("second call hits the cache"), Second, First);

	UUIBaseViewModel* Other = Controller->GetOrCreateViewModel(UUIBaseTestOtherViewModel::StaticClass());
	TestNotEqual(TEXT("a different class is a different instance"), Other, First);

	TestNull(TEXT("null class returns nullptr"), Controller->GetOrCreateViewModel(nullptr));
	TestNull(TEXT("abstract class returns nullptr"),
		Controller->GetOrCreateViewModel(UUIBaseViewModel::StaticClass()));

	Controller->ReleaseViewModel(UUIBaseTestViewModel::StaticClass());
	TestFalse(TEXT("released viewmodel was deinitialized"), First->IsInitialized());

	UUIBaseViewModel* AfterRelease = Controller->GetOrCreateViewModel(UUIBaseTestViewModel::StaticClass());
	TestNotEqual(TEXT("after release, a new instance"), AfterRelease, First);

	Controller->Deinitialize();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
