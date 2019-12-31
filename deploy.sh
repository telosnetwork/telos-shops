#! /bin/bash

#contract
if [[ "$1" == "directory" ]]; then
    contract=directory
else
    echo "need contract"
    exit 0
fi

#account
account=$2

#network
if [[ "$3" == "mainnet" ]]; then
    url=http://api.tlos.goodblock.io
    network="Telos Mainnet"
elif [[ "$3" == "testnet" ]]; then
    url=https://testnet.telosusa.io/
    network="Telos Testnet"
elif [[ "$3" == "local" ]]; then
    url=http://127.0.0.1:8888
    network="Local"
else
    echo "need network"
    exit 0
fi

echo ">>> Deploying $contract to $account on $network..."

# eosio v1.8.0
cleos -u $url set contract $account ./build/$contract/ $contract.wasm $contract.abi -p $account