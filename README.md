# cryptoart

Crypto and programmable artwork, using non-fungible token to represent and release in EOSIO blockchain.

Inspired by [async.art](https://async.art/).

## How to use

### 1. Create two basic artwork token symbol

We use `ART` as token symbol for cryptoart.

### 2. Call `mintartwork` to issue your art

The artist need to specify the uri of master token(usually IPFS cid of config data in JSON) to tell the [render](https://github.com/MobiusGame/crypto-art-render) how to cooperate with other layers to paint the final artwork at frontend.

The artist alse have to pass multiple collaborators to hold sub-layer tokens.

This action will issue NFTs to the specifed account, and after issuing, token holder can make effect on artwork by updating their layer token values.

At mobius crypto art, **we force all artworks only support IPFS storage network** to make more **decentralized**. The upper layers can also cache the buffer from IPFS for more efficient loading.

- For artwork with only 1 master, which often treated as normal artwork, its valid uri format is `mobius://crypto.art/ART/master?ipfs=${cid}`.
- For artwork with 1 master and multiple layers, its valid uri format is below:
  - Master Token: the same as above.
  - Layer Token: `mobius://crypto.art/ART/layer?master=${master_token_id}`, which will be generated automatically when mint artwork.

URI format follows the rule of [THE-OASIS URI Design](https://github.com/MobiusGame/nft-resolver#nft-%E5%85%83%E6%95%B0%E6%8D%AEmeta-data). To resolve URI more friendly, we recommand you to use [nft-resolver](https://github.com/MobiusGame/nft-resolver) SDK to decode the URI as object in Typescript.

### 3. Call `setuptoken` to setup the control token

Only token holders can setup and pass basic values set to their layer tokens.

- `min_values` - minimum value this token can have
- `max_values` - maximum value this token can have
- `curr_values` - current value the render load from

We map layer token id as the index of value array of master token.

### 4. Call `updatetoken` to update current value of your layer token

After setup tokens, you can also update current value of each layer token you hold. **This will make effect on the final artwork.**

## License

MIT
