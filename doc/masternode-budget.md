Masternode Budget Commands
==========================

BitBlocks Core includes budget RPC commands for proposal preparation, submission, voting, and projection.

Common commands
---------------

Use a budget-cycle-aligned start block. Mainnet budget cycles are `43,200` blocks, so examples use `820800`.

```bash
bitblocks-cli walletpassphrase "your passphrase" 120
bitblocks-cli preparebudget "proposal-name" "https://example.com/proposal" 12 820800 "BExamplePayoutAddress" 500
bitblocks-cli submitbudget "proposal-name" "https://example.com/proposal" 12 820800 "BExamplePayoutAddress" 500 "fee-txid"
bitblocks-cli getbudgetprojection
bitblocks-cli mnbudgetvote many "proposal-hash" yes
```

Legacy wrapper
--------------

Some releases also expose the `mnbudget` wrapper. Prefer the direct RPC commands above for new scripts, but legacy forms may still work for compatibility.

Proposal fields
---------------

- `proposal-name`: short unique name, maximum `20` characters.
- `url`: public proposal details, maximum `64` characters.
- `payment-count`: number of payments requested.
- `block-start`: first payment block; it must be aligned to the budget payment cycle.
- `payment-address`: BitBlocks payout address.
- `monthly-payment`: requested amount per payment.
- `fee-txid`: transaction id returned by `preparebudget`.

Voting
------

Masternode operators can vote from the controller wallet after their masternodes are active:

```bash
bitblocks-cli mnbudgetvote many "proposal-hash" yes
bitblocks-cli mnbudgetvote many "proposal-hash" no
```

Use the proposal hash returned by `submitbudget`, not the fee transaction id returned by `preparebudget`.

Review projections before voting:

```bash
bitblocks-cli getbudgetprojection
```

Security notes
--------------

- Verify proposal URLs before voting.
- Keep controller wallets encrypted and locked when not signing.
- Do not paste private keys into public proposal discussions.
