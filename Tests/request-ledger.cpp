#include "../Source/Nuxie/Private/Core/RequestLedger.h"
#include <cassert>
#include <iostream>
int main() {
  using namespace nuxie;
  assert(counter("18446744073709551615") == UINT64_MAX);
  assert(!counter("18446744073709551616"));
  assert(!counter("-1") && !counter("1e3") && !counter("01") && !counter(""));
  assert(quantity(1) && quantity(9007199254740991LL));
  assert(!quantity(0) && !quantity(-1) && !quantity(9007199254740992LL));
  RequestLedger ledger;
  assert(ledger.admit("query", {1, false, 90}));
  assert(!ledger.admit("query", {2, false, 200}));
  assert(ledger.expired(89).empty());
  assert(ledger.expired(90) == std::vector<std::string>{"query"});
  auto query = ledger.take("query"); assert(query && query->identity == 1);
  assert(!RequestLedger::deliverable(*query, 2));
  assert(!ledger.take("query")); // A late/duplicate completion cannot settle again.
  assert(ledger.admit("spend", {1, true, 90}));
  auto receipt = ledger.take("spend"); assert(receipt && RequestLedger::deliverable(*receipt, 2));
  assert(ledger.all().empty());
  std::cout << "Request correlation, numeric fidelity, timeout and identity tests passed\n";
}
