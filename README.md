# cryptoart

Crypto and programmable artwork, using non-fungible token to represent and release in EOSIO blockchain.

Inspired by [async.art](https://async.art/).

## How to use

### 1. Create two basic artwork token symbol

- `NORMAL` - means normal artwork token with only 1 **master** layer.
- `PROGRAM` - means programmable artwork token with 1 master layer and multiple sub layers controlled by other token-holders.

### 2. Call `mintartwork` to issue your art

The artist need to specify the uri of master token(usually IPFS cid of config data in JSON) to tell the [render](https://github.com/MobiusGame/crypto-art-render) how to cooperate with other layers to paint the final artwork at frontend.

The artist alse have to pass multiple collaborators to hold sub-layer tokens.

This action will issue NFTs to the specifed account, and after issuing, token holder can make effect on artwork by updating their layer token values.

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
