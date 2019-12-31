// A directory contract for EOSIO software.
//
// author: Craig Branscom
// contract: directory
// version: v0.2.0
// date: December 3rd, 2019

#include <eosio/eosio.hpp>
#include <eosio/action.hpp>
#include <eosio/singleton.hpp>
#include <eosio/asset.hpp>

using namespace std;
using namespace eosio;

CONTRACT directory : public contract {

    public:

    directory(name self, name code, datastream<const char*> ds);

    ~directory();

    //constants
    const name ADMIN_NAME = name("tlsdirectory");
    const symbol TLOS_SYM = symbol("TLOS", 4);

    //dapp statuses: submitted, approved, rejected

    //dapp platforms: ios, android, mac, linux, windows, web

    //fees: submitdapp = 50 TLOS

    //categories: games, finance, music, developer, 

    //======================== admin actions ========================

    //initializes the contract
    ACTION init(name initial_admin);

    //sets a new contract version
    ACTION setversion(string new_version);

    //sets a new admin
    ACTION setadmin(name new_admin, string memo);

    //updates or inserts a fee
    ACTION upsertfee(name fee_name, asset new_fee);

    //removes a fee
    ACTION rmvfee(name fee_name);

    //review a dapp submission
    ACTION reviewdapp(name dapp_account, bool approve, string memo);

    //add dapp to featured list
    ACTION addfeatured(uint16_t slot_number, name dapp_account, time_point_sec featured_until);

    //remove dapp from featured list
    ACTION rmvfeatured(uint16_t slot_number);

    //pay cpu and net cost for contract trx
    ACTION payforbw();

    //======================== dapp actions ========================

    //submit a dapp for approval
    ACTION submitdapp(string title, string subtitle, string description, string website, string version,
        name dapp_account, name manager, name category);

    //update dapp title, subtitle, description, website, version
    ACTION updateinfo(name dapp_account, optional<string> new_title, optional<string> new_subtitle, 
        optional<string> new_description, optional<string> new_website, optional<string> new_version);

    //update dapp icons
    ACTION updateicons(name dapp_account, optional<string> icon_small, optional<string> icon_large);

    //update dapp slides
    ACTION updateslides(name dapp_account, vector<string> new_slides);

    //set dapp platforms
    ACTION setplatforms(name dapp_account, map<name, string> new_platforms);

    //change dapp manager
    ACTION chmanager(name dapp_account, name new_manager, string memo);

    //delete dapp entry
    ACTION deletedapp(name dapp_account, string memo);

    //======================== vending actions ========================

    //add new vending item
    ACTION regitem(string title, string subtitle, name dapp_account, name item_name, asset price, uint32_t stock);

    //make payment for item and notify contract account to vend item
    ACTION purchase(name purchaser, name item_name, name dapp_account);

    //restock item
    ACTION restock(name item_name, name dapp_account, uint32_t new_stock);

    //remove an item
    ACTION rmvitem(name item_name, name dapp_account);

    //======================== account actions ========================

    //withdraw TLOS from directory account
    ACTION withdraw(name account_owner, asset quantity);

    //========== notification methods ==========

    //catches TLOS transfers from eosio.token
    [[eosio::on_notify("eosio.token::transfer")]]
    void catch_tlos_transfer(name from, name to, asset quantity, string memo);

    //========== utility methods ==========

    //requires a charge to an account
    void require_fee(name account_owner, asset quantity);

    //validates a category
    bool valid_category(name category_name);

    //validates a dapp platform
    bool valid_platform(name platform_name);

    //======================== contract tables ========================

    //contract config
    //scope: singleton
    //ram: 
    TABLE config {
        string version; //v0.2.0
        name admin;
        map<name, asset> fees; //fee_name => fee_amount

        EOSLIB_SERIALIZE(config, (version)(admin)(fees))
    };
    typedef singleton<name("config"), config> config_singleton;

    //dapp entry
    //scope: self
    //ram: 
    TABLE dapp {
        name dapp_account;
        name manager;
        name category;
        name status;
    
        string icon_small; //16x16
        string icon_large; //64x64
        string title;
        string subtitle;
        string description;
        string website;
        string version;
        vector<string> slides;
        map<name, string> platforms; //platform_name => download_link

        time_point_sec last_updated;

        uint64_t primary_key() const { return dapp_account.value; }
        uint64_t by_manager() const { return manager.value; }
        uint64_t by_category() const { return category.value; }
        EOSLIB_SERIALIZE(dapp, 
            (dapp_account)(manager)(category)(status)
            (icon_small)(icon_large)(title)(subtitle)(description)(website)(version)(slides)(platforms)
            (last_updated))
    };
    typedef multi_index<name("dapps"), dapp,
        indexed_by<name("bymanager"), const_mem_fun<dapp, uint64_t, &dapp::by_manager>>,
        indexed_by<name("bycategory"), const_mem_fun<dapp, uint64_t, &dapp::by_category>>
    > dapps_table;

    //account balance
    //scope: owner.value
    //ram: ~300B
    TABLE account {
        asset balance;

        uint64_t primary_key() const { return balance.symbol.code().raw(); }
        EOSLIB_SERIALIZE(account, (balance))
    };
    typedef multi_index<name("accounts"), account> accounts_table;

    //prebuilt in-dapp item payment
    //scope: dapp_account.value
    //ram: 
    TABLE item {
        name item_name;
        string title;
        string subtitle;
        asset price;
        uint32_t stock;

        uint64_t primary_key() const { return item_name.value; }
        EOSLIB_SERIALIZE(item, (item_name)(title)(subtitle)(price)(stock))
    };
    typedef multi_index<name("items"), item> items_table;

    //featured dapps
    //scope: self
    TABLE featured_slot {
        uint64_t slot_number;
        name featured_dapp;
        time_point_sec featured_until;

        uint64_t primary_key() const { return slot_number; }
        EOSLIB_SERIALIZE(featured_slot, (slot_number)(featured_dapp)(featured_until))
    };
    typedef multi_index<"featured"_n, featured_slot> featured_table;

};