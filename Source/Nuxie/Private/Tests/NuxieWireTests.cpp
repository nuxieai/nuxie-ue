#include "Misc/AutomationTest.h"
#include <limits>
#if WITH_DEV_AUTOMATION_TESTS
#include "NuxieWire.h"
#include "NuxieBlueprintLibrary.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNuxieWireContract, "Nuxie.Contract.ValuesAndReceipts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNuxieWireContract::RunTest(const FString&) {
  for (const TCHAR* Name : { TEXT("appAction"), TEXT("activity"), TEXT("purchase"), TEXT("restore"), TEXT("features") }) {
    auto Event = NuxieWire::Object(TEXT("{\"session\":\"current\",\"identityGeneration\":\"2\"}"));
    Event->SetStringField(TEXT("name"), Name);
    TestTrue(TEXT("current customer event admitted"), NuxieWire::EventMatchesIdentity(Event, TEXT("current"), TEXT("2"), false));
    TestFalse(TEXT("queued old customer event rejected after identify"), NuxieWire::EventMatchesIdentity(Event, TEXT("current"), TEXT("3"), false));
    TestFalse(TEXT("event rejected while identity is changing"), NuxieWire::EventMatchesIdentity(Event, TEXT("current"), TEXT("2"), true));
    TestFalse(TEXT("event rejected from old game session"), NuxieWire::EventMatchesIdentity(Event, TEXT("replacement"), TEXT("2"), false));
    Event->RemoveField(TEXT("identityGeneration"));
    TestFalse(TEXT("missing provenance fails closed"), NuxieWire::EventMatchesIdentity(Event, TEXT("current"), TEXT("2"), false));
  }
  FNuxieProperties Properties; FString Error;
  TestTrue(TEXT("nested portable values"), FNuxieProperties::TryParse(TEXT("{\"name\":\"🎮 café\",\"items\":[true,null,{\"n\":9007199254740991}]}"), Properties, Error));
  const FString Before = Properties.ToJson();
  TestFalse(TEXT("reject out of range integer"), FNuxieProperties::TryParse(TEXT("{\"n\":9007199254740992}"), Properties, Error));
  TestEqual(TEXT("failed parse preserves value"), Properties.ToJson(), Before);
  TestFalse(TEXT("root must be object"), FNuxieProperties::TryParse(TEXT("[]"), Properties, Error));
  FNuxieJsonValue Number;
  TestFalse(TEXT("reject infinity"), UNuxieBlueprintLibrary::NumberValue(std::numeric_limits<double>::infinity(), Number, Error));
  TestTrue(TEXT("typed integer maximum accepted"), Properties.WithInteger(TEXT("count"), 9007199254740991LL, Error));
  const FString TypedBefore = Properties.ToJson();
  TestFalse(TEXT("integer bounds checked before double conversion"), Properties.WithInteger(TEXT("count"), MAX_int64, Error));
  TestEqual(TEXT("failed builder preserves properties"), Properties.ToJson(), TypedBefore);
  TestTrue(TEXT("typed string escapes JSON"), Properties.WithString(TEXT("quoted"), TEXT("a\"b"), Error));
  TestEqual(TEXT("typed string round trips"), NuxieWire::Object(Properties.ToJson())->GetStringField(TEXT("quoted")), FString(TEXT("a\"b")));
  TestTrue(TEXT("typed boolean"), Properties.WithBool(TEXT("enabled"), true, Error));
  TestTrue(TEXT("typed null"), Properties.WithNull(TEXT("empty"), Error));
  TArray<FNuxieJsonValue> Items = { UNuxieBlueprintLibrary::StringValue(TEXT("value")), UNuxieBlueprintLibrary::NullValue() };
  TestTrue(TEXT("typed scalar array"), Properties.WithArray(TEXT("items"), Items, Error));
  auto Built = NuxieWire::Object(Properties.ToJson());
  TestTrue(TEXT("boolean round trips"), Built && Built->GetBoolField(TEXT("enabled")));
  TestTrue(TEXT("null round trips"), Built && Built->GetField<EJson::Null>(TEXT("empty"))->IsNull());
  TestEqual(TEXT("array contains both values"), Built->GetArrayField(TEXT("items")).Num(), 2);
  FNuxieJsonValue Parsed;
  TestFalse(TEXT("multiple root values rejected"), FNuxieJsonValue::TryParse(TEXT("1,2"), Parsed, Error));
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
