#include "Misc/AutomationTest.h"
#include <limits>
#if WITH_DEV_AUTOMATION_TESTS
#include "NuxieWire.h"
#include "NuxieBlueprintLibrary.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNuxieWireContract, "Nuxie.Contract.ValuesAndReceipts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNuxieWireContract::RunTest(const FString&) {
  FNuxieProperties Properties; FString Error;
  TestTrue(TEXT("nested portable values"), FNuxieProperties::TryParse(TEXT("{\"name\":\"🎮 café\",\"items\":[true,null,{\"n\":9007199254740991}]}"), Properties, Error));
  const FString Before = Properties.ToJson();
  TestFalse(TEXT("reject out of range integer"), FNuxieProperties::TryParse(TEXT("{\"n\":9007199254740992}"), Properties, Error));
  TestEqual(TEXT("failed parse preserves value"), Properties.ToJson(), Before);
  TestFalse(TEXT("root must be object"), FNuxieProperties::TryParse(TEXT("[]"), Properties, Error));
  FNuxieJsonValue Number;
  TestFalse(TEXT("reject infinity"), UNuxieBlueprintLibrary::NumberValue(std::numeric_limits<double>::infinity(), Number, Error));
  auto AccessObject = NuxieWire::Object(TEXT("{\"allowed\":false,\"unlimited\":false,\"balance\":0,\"type\":\"metered\"}"));
  FNuxieFeatureAccess Access;
  TestTrue(TEXT("zero access decodes"), NuxieWire::Access(AccessObject, Access));
  TestTrue(TEXT("zero is present"), Access.bHasBalance);
  TestFalse(TEXT("zero is not unlimited"), Access.bUnlimited);
  auto ReceiptObject = NuxieWire::Object(TEXT("{\"customerId\":\"a\",\"featureId\":\"coins\",\"operationId\":\"saved\",\"quantity\":1,\"occurredAtMs\":null,\"accepted\":true,\"code\":\"accepted\",\"balance\":0,\"unlimited\":false,\"active\":true,\"idempotentReplay\":true}"));
  FNuxieUsageReceipt Receipt;
  TestTrue(TEXT("authoritative receipt decodes"), NuxieWire::Receipt(ReceiptObject, Receipt));
  TestTrue(TEXT("accepted last unit stays accepted"), Receipt.bAccepted);
  TestTrue(TEXT("replay retained"), Receipt.bIdempotentReplay);
  ReceiptObject->SetNumberField(TEXT("quantity"), 1.5);
  TestFalse(TEXT("fractional command rejected"), NuxieWire::Receipt(ReceiptObject, Receipt));
  FNuxieFeatureSnapshot Snapshot;
  TestTrue(TEXT("full unsigned generation preserved"), NuxieWire::Snapshot(NuxieWire::Object(TEXT("{\"identityGeneration\":\"18446744073709551615\",\"revision\":\"9007199254740993\",\"state\":\"unknown\",\"all\":{}}")), Snapshot));
  TestEqual(TEXT("revision is exact"), Snapshot.Revision, FString(TEXT("9007199254740993")));
  TestTrue(TEXT("unknown is explicit"), Snapshot.Kind == ENuxieFeatureStateKind::Unknown);
  TMap<FString, FNuxieScalar> Scalars;
  TestTrue(TEXT("native signed limits decode without doubles"), NuxieWire::Scalars(NuxieWire::Object(TEXT("{\"minimum\":{\"kind\":\"integer\",\"value\":\"-9223372036854775808\"},\"maximum\":{\"kind\":\"integer\",\"value\":\"9223372036854775807\"}}")), Scalars));
  TestEqual(TEXT("minimum exact"), Scalars[TEXT("minimum")].Integer, MIN_int64);
  TestEqual(TEXT("maximum exact"), Scalars[TEXT("maximum")].Integer, MAX_int64);
  FNuxieStoreProduct Product;
  auto ProductObject = NuxieWire::Object(TEXT("{\"platform\":\"android\",\"productId\":\"logical\",\"storeProductId\":\"store\",\"placementId\":\"shop\",\"offerId\":null,\"displayPrice\":\"\",\"pricingPhases\":[]}"));
  TestTrue(TEXT("selected product decodes"), NuxieWire::Product(ProductObject, Product));
  TestFalse(TEXT("null offer is absent"), Product.bHasOfferId);
  TestTrue(TEXT("empty display price remains present"), Product.bHasDisplayPrice);
  TestTrue(TEXT("empty phases remain present"), Product.bHasPricingPhases);
  ProductObject->SetStringField(TEXT("pricingPhases"), TEXT("invalid"));
  TestFalse(TEXT("malformed selected offer is rejected"), NuxieWire::Product(ProductObject, Product));
  return true;
}
#endif
