#!/usr/bin/env python3
"""Check a real device Lab controller report against the public completion contract."""
import argparse
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('report', type=Path)
parser.add_argument('--kind', required=True, choices=['purchase', 'restore'])
parser.add_argument('--outcome', required=True)
parser.add_argument('--store-product-id')
parser.add_argument('--base-plan-id')
args = parser.parse_args()
outcomes = {
    'purchase': {'purchased', 'cancelled', 'pending', 'failed'},
    'restore': {'restored', 'noPurchases', 'failed'},
}
if args.outcome not in outcomes[args.kind]:
    parser.error('The selected outcome does not belong to this request kind.')
report = json.loads(args.report.read_text())
expected = {
    'kind': args.kind,
    'outcome': args.outcome,
    'simulatedControllerOutcome': True,
    'invalidOutcomeRejected': True,
    'pendingAfterInvalid': True,
    'completionAccepted': True,
    'duplicateRejected': True,
    'pendingAfterCompletion': False,
}
if args.store_product_id is not None:
    expected['storeProductId'] = args.store_product_id
if args.base_plan_id is not None:
    expected['hasBasePlanId'] = True
    expected['basePlanId'] = args.base_plan_id
failures = {
    key: {'expected': value, 'actual': report.get(key)}
    for key, value in expected.items()
    if type(report.get(key)) is not type(value) or report.get(key) != value
}
print(json.dumps({'timestamp': report.get('timestamp'), 'failures': failures}))
raise SystemExit(bool(failures))
