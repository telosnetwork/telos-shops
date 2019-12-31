#include "../include/directory.hpp"

directory::directory(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {}

directory::~directory() {}

//======================== admin actions ========================

ACTION directory::init(name initial_admin) {

    //authenticate
    require_auth(get_self());

    //open config and featured singletons
    config_singleton configs(get_self(), get_self().value);

    //validate
    check(!configs.exists(), "configs already initialized");
    check(is_account(initial_admin), "initial admin account doesn't exist");

    //initialize
    map<name, asset> initial_fees;
    initial_fees["submitdapp"_n] = asset(500000, TLOS_SYM); //50 TLOS
    initial_fees["regitem"_n] = asset(50000, TLOS_SYM); //5 TLOS

    config initial_configs = {
        string("v0.2.0"), //version
        initial_admin, //admin
        initial_fees //fees
    };

    //set initial config
    configs.set(initial_configs, get_self());

}

ACTION directory::setversion(string new_version) {

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //set new version
    conf.version = new_version;

    //set config
    configs.set(conf, get_self());

}

ACTION directory::setadmin(name new_admin, string memo) {

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //set new admin
    conf.admin = new_admin;

    //set config
    configs.set(conf, get_self());

}

ACTION directory::upsertfee(name fee_name, asset new_fee) {
    
    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //set new fee
    conf.fees[fee_name] = new_fee;

    //set config
    configs.set(conf, get_self());
    
}

ACTION directory::rmvfee(name fee_name) {

    //open confgi singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //erase fee
    conf.fees.erase(fee_name);

    //set new config
    configs.set(conf, get_self());

}

ACTION directory::reviewdapp(name dapp_account, bool approve, string memo) {

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //open dapps table, get dapp submission
    dapps_table dapps(get_self(), get_self().value);
    auto& d = dapps.get(dapp_account.value, "dapp submission not found");

    //initialize
    name new_status;

    if (approve) {
        new_status = "approved"_n;
    } else {
        new_status = "rejected"_n;
    }

    //update dapp status
    dapps.modify(d, same_payer, [&](auto& col) {
        col.status = new_status;
    });

}

ACTION directory::addfeatured(uint16_t slot_number, name dapp_account, time_point_sec featured_until) {

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //open featured table, get featured slot
    featured_table featured(get_self(), get_self().value);
    auto f = featured.find(slot_number);

    if (f == featured.end()) { //not found
        //emplace featured slot
        featured.emplace(get_self(), [&](auto& col) {
            col.slot_number = slot_number;
            col.featured_dapp = dapp_account;
            col.featured_until = featured_until;
        });
    } else { //found
        //update featured slot
        featured.modify(f, same_payer, [&](auto& col) {
            col.slot_number = slot_number;
            col.featured_dapp = dapp_account;
            col.featured_until = featured_until;
        });
    }

}

ACTION directory::rmvfeatured(uint16_t slot_number) {

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //open featured table, get featured slot
    featured_table featured(get_self(), get_self().value);
    auto& f = featured.get(slot_number, "featured slot not found");

    //erase featured slot
    featured.erase(f);

}

ACTION directory::payforbw() {

    //authenticate
    require_auth(get_self());

}

//======================== dapp actions ========================

ACTION directory::submitdapp(string title, string subtitle, string description, string website, string version,
    name dapp_account, name manager, name category) {
    
    //authenticate
    require_auth(dapp_account);

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //charge fee
    require_fee(dapp_account, conf.fees.at("submitdapp"_n));

    //initialize
    vector<string> blank_slides;
    map<name, string> initial_platforms;
    time_point_sec now = time_point_sec(current_time_point());

    //open tests table, search for account name
    dapps_table dapps(get_self(), get_self().value);
    auto d = dapps.find(dapp_account.value);

    //validate
    check(d == dapps.end(), "this account already has a dapp");
    check(is_account(manager), "manager account doesn't exist");
    check(valid_category(category), "invalid category");

    //emplace dapp entry, ram paid by contract
    dapps.emplace(get_self(), [&](auto& col) {
        col.dapp_account = dapp_account;
        col.manager = manager;
        col.category = category;
        col.status = name("submitted");
        col.icon_small = "";
        col.icon_large = "";
        col.title = title;
        col.subtitle = subtitle;
        col.description = description;
        col.website = website;
        col.version = version;
        col.slides = blank_slides;
        col.platforms = initial_platforms;
        col.last_updated = now;
    });

}

ACTION directory::updateinfo(name dapp_account, optional<string> new_title, optional<string> new_subtitle, 
    optional<string> new_description, optional<string> new_website, optional<string> new_version) {

    //open dapps table, search for dapp
    dapps_table dapps(get_self(), get_self().value);
    auto& d = dapps.get(dapp_account.value, "dapp not found");

    //authenticate
    require_auth(d.manager);

    //initialize
    string title;
    string subtitle;
    string description;
    string website;
    string version;
    time_point_sec now = time_point_sec(current_time_point());

    if (new_title) {
        title = *new_title;
    } else {
        title = d.title;
    }

    if (new_subtitle) {
        subtitle = *new_subtitle;
    } else {
        subtitle = d.subtitle;
    }

    if (new_description) {
        description = *new_description;
    } else {
        description = d.description;
    }

    if (new_website) {
        website = *new_website;
    } else {
        website = d.website;
    }

    if (new_version) {
        version = *new_version;
    } else {
        version = d.version;
    }

    //update dapp info
    dapps.modify(d, same_payer, [&](auto& col) {
        col.title = title;
        col.subtitle = subtitle;
        col.description = description;
        col.website = website;
        col.version = version;
    });

}

ACTION directory::updateicons(name dapp_account, optional<string> new_icon_small, optional<string> new_icon_large) {

    //open dapps table, search for dapp
    dapps_table dapps(get_self(), get_self().value);
    auto& d = dapps.get(dapp_account.value, "dapp not found");

    //authenticate
    require_auth(d.manager);

    //initialize
    string icon_small;
    string icon_large;
    time_point_sec now = time_point_sec(current_time_point());

    if (new_icon_small) {
        icon_small = *new_icon_small;
    } else {
        icon_small = d.icon_small;
    }

    if (new_icon_large) {
        icon_large = *new_icon_large;
    } else {
        icon_large = d.icon_large;
    }

    //update dapp icons
    dapps.modify(d, same_payer, [&](auto& col) {
        col.icon_small = icon_small;
        col.icon_large = icon_large;
        col.last_updated = now;
    });

}

ACTION directory::updateslides(name dapp_account, vector<string> new_slides) {
    
    //open dapps table, search for dapp
    dapps_table dapps(get_self(), get_self().value);
    auto& d = dapps.get(dapp_account.value, "dapp not found");

    //authenticate
    require_auth(d.manager);

    //validate
    check(new_slides.size() <= 0, "cannot have more than 5 slides");

    //initialize
    time_point_sec now = time_point_sec(current_time_point());

    //update dapp slides
    dapps.modify(d, same_payer, [&](auto& col) {
        col.slides = new_slides;
        col.last_updated = now;
    });

}

ACTION directory::setplatforms(name dapp_account, map<name, string> new_platforms) {

    //open dapps table, search for dapp
    dapps_table dapps(get_self(), get_self().value);
    auto& d = dapps.get(dapp_account.value, "dapp not found");

    //authenticate
    require_auth(d.manager);

    //validate
    check(new_platforms.size() > 0, "must submit at least 1 platform");
    
    for (auto itr = new_platforms.begin(); itr != new_platforms.end(); itr++) {

        check(valid_platform(itr->first), "invalid platform");

    }

    //initialize
    time_point_sec now = time_point_sec(current_time_point());

    //update dapp platforms
    dapps.modify(d, same_payer, [&](auto& col) {
        col.platforms = new_platforms;
        col.last_updated = now;
    });

}

ACTION directory::chmanager(name dapp_account, name new_manager, string memo) {

    //open dapps table, search for dapp
    dapps_table dapps(get_self(), get_self().value);
    auto& d = dapps.get(dapp_account.value, "dapp not found");

    //authenticate
    require_auth(d.manager);

    //validate
    check(is_account(new_manager), "new manager account doesn't exist");

    //initialize
    time_point_sec now = time_point_sec(current_time_point());

    //update dapp manager
    dapps.modify(d, same_payer, [&](auto& col) {
        col.manager = new_manager;
        col.last_updated = now;
    });

}

ACTION directory::deletedapp(name dapp_account, string memo) {

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //open dapps table, search for dapp
    dapps_table dapps(get_self(), get_self().value);
    auto& d = dapps.get(dapp_account.value, "dapp not found");

    if (d.status == "rejected"_n) {
        //authenticate
        require_auth(conf.admin);
    } else {
        //authenticate
        require_auth(d.manager);
    }

    //erase dapp entry
    dapps.erase(d);

}

//======================== purchasable actions ========================

ACTION directory::regitem(string title, string subtitle, name dapp_account, name item_name, asset price, uint32_t stock) {

    //open dapps table, get dapp
    dapps_table dapps(get_self(), get_self().value);
    auto& d = dapps.get(dapp_account.value, "dapp not found");

    //open items table, find item
    items_table items(get_self(), dapp_account.value);
    auto i = items.find(item_name.value);

    //authenticate
    require_auth(d.manager);

    //validate
    check(i == items.end(), "tem already exists");
    check(price.symbol == TLOS_SYM, "price must be denominated in TLOS");
    check(price.amount > 0, "price amount must be greater than 0");
    check(stock > 0, "stock must be a positive number");

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //charge fee
    require_fee(d.manager, conf.fees.at("regitem"_n));

    //emplace new dapp item
    items.emplace(d.manager, [&](auto& col) {
        col.item_name = item_name;
        col.title = title;
        col.subtitle = subtitle;
        col.price = price;
        col.stock = stock;
    });

}

ACTION directory::purchase(name purchaser, name item_name, name dapp_account) {

    //authenticate
    require_auth(purchaser);

    //open dapps table, get dapp
    dapps_table dapps(get_self(), get_self().value);
    auto& d = dapps.get(dapp_account.value, "dapp not found");

    //open items table, get item
    items_table items(get_self(), dapp_account.value);
    auto& i = items.get(item_name.value, "item not found");

    //charge price to purchaser account
    require_fee(purchaser, i.price);

    //open accounts table, get account
    accounts_table accounts(get_self(), dapp_account.value);
    auto& a = accounts.get(TLOS_SYM.code().raw(), "account not found");

    //validate
    check(i.stock > 0, "stock is empty");

    //decrement item stock
    items.modify(i, same_payer, [&](auto& col) {
        col.stock -= 1;
    });

    //deposit item price to dapp account
    accounts.modify(a, same_payer, [&](auto& col) {
        col.balance += i.price;
    });

    //notify contract account of purchase
    require_recipient(dapp_account);

}

ACTION directory::restock(name item_name, name dapp_account, uint32_t new_stock) {

    //open dapps table, get dapp
    dapps_table dapps(get_self(), get_self().value);
    auto& d = dapps.get(dapp_account.value, "dapp not found");

    //authenticate
    require_auth(d.manager);

    //open items table, get item
    items_table items(get_self(), dapp_account.value);
    auto& i = items.get(item_name.value, "item not found");

    //update item stock
    items.modify(i, same_payer, [&](auto& col) {
        col.stock = new_stock;
    });

}

ACTION directory::rmvitem(name item_name, name dapp_account) {

    //open dapps table, get dapp
    dapps_table dapps(get_self(), get_self().value);
    auto& d = dapps.get(dapp_account.value, "dapp not found");

    //authenticate
    require_auth(d.manager);

    //open items table, get item
    items_table items(get_self(), dapp_account.value);
    auto& i = items.get(item_name.value, "item not found");

    //erase item
    items.erase(i);

}

//======================== account actions ========================

ACTION directory::withdraw(name account_owner, asset quantity) {

    //authenticate
    require_auth(account_owner);

    //open accounts table, get account
    accounts_table accounts(get_self(), account_owner.value);
    auto& acct = accounts.get(TLOS_SYM.code().raw(), "account not found");

    //validate
    check(acct.balance >= quantity, "insufficient balance");
    check(quantity.amount > 0, "must withdraw a positive amount");

    //charge withdrawal amount to account owner
    accounts.modify(acct, same_payer, [&](auto& col) {
        col.balance -= quantity;
    });

    //send inline to eosio.token
    action(permission_level{get_self(), name("active")}, name("eosio.token"), name("transfer"), make_tuple(
		get_self(), //from
		account_owner, //to
		quantity, //quantity
        std::string("dapp store withdrawal") //memo
	)).send();

}

//========== notification methods ==========

void directory::catch_tlos_transfer(name from, name to, asset quantity, string memo) {

    //get initial receiver contract
    name rec = get_first_receiver();

    //validate
    if (rec == name("eosio.token") && from != get_self() && quantity.symbol == TLOS_SYM) {
        
        //parse memo
        if (memo == std::string("skip")) {
            return;
        } else {

            //open accounts table, search for account
            accounts_table accounts(get_self(), from.value);
            auto acct = accounts.find(TLOS_SYM.code().raw());

            //emplace account if not found, update if exists
            if (acct == accounts.end()) { //no account
                accounts.emplace(get_self(), [&](auto& col) {
                    col.balance = quantity;
                });
            } else { //exists
                accounts.modify(*acct, same_payer, [&](auto& col) {
                    col.balance += quantity;
                });
            }
        }
    }
}

//========== utility methods ==========

void directory::require_fee(name account_owner, asset quantity) {

    //open accounts table, get account
    accounts_table accounts(get_self(), account_owner.value);
    auto& acct = accounts.get(quantity.symbol.code().raw(), "require_fee: account not found");

    //validate
    check(acct.balance >= quantity, "require_fee: insufficient funds");

    //update account balance
    accounts.modify(acct, same_payer, [&](auto& col) {
        col.balance -= quantity;
    });

}

bool directory::valid_category(name category_name) {

    switch (category_name.value) {
        case (name("games").value):
            return true;
        case (name("finance").value):
            return true;
        case (name("music").value):
            return true;
        case (name("developer").value):
            return true;
        default:
            return false;
    }

}

bool directory::valid_platform(name platform_name) {

    switch (platform_name.value) {
        case (name("ios").value):
            return true;
        case (name("android").value):
            return true;
        case (name("mac").value):
            return true;
        case (name("linux").value):
            return true;
        case (name("windows").value):
            return true;
        case (name("web").value):
            return true;
        default:
            return false;
    }

}
