# Telos Apps
Telos Apps is an App Storefront and Directory for the Telos Blockchain Network.

## Features

### `Dapp Approval and Publication`

Submit a dapp for publication. After approval, dapps can update their saved information at any time. Dapps can manage their dapp page with custom icons, banner images, preview slides, numerous badges, and more.

### `Robust Admin Controls`

Directory operators can manage admin rights, such as dapp approval, category management, and fee adjustment, to cater the platform to their use case.

### `Deposit/Spend/Withdraw Accounting`

The directory contract comes pre-built with a secure accounting mechanism enabling a complete deposit/spend/withdraw lifecycle.

## Roadmap

* On-Chain Tags for Search Classification
* Featured Dapps table with competitive ranking

# Contract Setup, Building, and Deployment

## Prerequisites

* eosio 1.8.x
* eosio.cdt 1.6.x

## Setup

To begin, navigate to the project directory: `eosio-directory/`

    mkdir build && mkdir build/directory

    chmod +x build.sh

    chmod +x deploy.sh

Ensure the binaries for the `eosio.cdt` and `eosio` are either aliased or set in your PATH.

## Build

    ./build.sh directory

## Deploy

    ./deploy.sh directory { mainnet | testnet | local }
