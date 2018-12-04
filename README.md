# bts-ws-client

## build:

gcc -g -O2 -o bts-wscli bts-wscli.c sha1.c base64.c

gcc -g -O2 -o bts-wscli-ssl bts-wscli-ssl.c sha1.c base64.c -lssl


## run
./bts-wscli-ssl wss://bit.btsabc.org/ws


request[1]: {"id":1,"method":"call","params":[1,"login",["",""]]}
return: (len=38): {"id":1,"jsonrpc":"2.0","result":true}


request[2]: {"id":2,"method":"call","params":[1,"database",[]]}

return: (len=35): {"id":2,"jsonrpc":"2.0","result":2}



request[3]: {"id":3,"method":"call","params":[1,"network_broadcast",[]]}

return: (len=35): {"id":3,"jsonrpc":"2.0","result":3}



request[4]: {"id":4,"method":"call","params":[1,"history",[]]}

return: (len=35): {"id":4,"jsonrpc":"2.0","result":4}



request[5]: {"id":5,"method":"call","params":[2, "get_chain_id", []]}

return: (len=100): {"id":5,"jsonrpc":"2.0","result":"dbbf214b088010baf317abe51b734b514490616dda48669b2bea8a1de6ff49c3"}



request[6]: {"id":6,"method":"call","params":[2, "get_global_properties", []]}

return: (len=2672): {"id":6,"jsonrpc":"2.0","result":{"id":"2.0.0","parameters":{"current_fees":{"parameters":[[0,{"fee":2000000,"price_per_kbyte":1000000}],[1,{"basic_fee":500000,"premium_fee":200000000,"price_per_kbyte":100000}],[2,{"fee":2000000,"price_per_kbyte":100000}],[3,{"fee":300000}],[4,{"membership_annual_fee":200000000,"membership_lifetime_fee":1000000000}],[5,{"fee":50000000}],[6,{"symbol3":"50000000000","symbol4":"30000000000","long_symbol":500000000,"price_per_kbyte":10}],[7,{"fee":50000000,"price_per_kbyte":10}],[8,{"fee":50000000}],[9,{"fee":2000000,"price_per_kbyte":100000}],[10,{"fee":2000000}],[11,{"fee":100000}],[12,{"fee":500000000}],[13,{"fee":2000000}],[14,{"fee":2000000,"price_per_kbyte":10}],[15,{"fee":2000000,"price_per_kbyte":10}],[16,{"fee":100000}],[17,{"fee":100000}],[18,{"fee":100000}],[19,{"fee":2000000,"price_per_kbyte":10}],[20,{"fee":0}],[21,{"fee":500000000}],[22,{"fee":2000000}],[23,{"fee":100000}],[24,{"fee":100000}],[25,{"fee":2000000}],[26,{"fee":500000000}],[27,{"fee":100000,"price_per_kbyte":10}],[28,{"fee":100000}],[29,{}],[30,{"fee":2000000,"price_per_kbyte":10}],[31,{"fee":500000,"price_per_output":500000}],[32,{"fee":500000,"price_per_output":500000}],[33,{"fee":500000}],[34,{"fee":2000000}],[35,{}],[36,{"fee":100000,"price_per_kbyte":100000}],[37,{"fee":100,"price_per_kbyte_ram":50000,"price_per_ms_cpu":0}],[38,{"fee":2000000,"price_per_kbyte":1000000}],[39,{"fee":2000000,"price_per_kbyte":1000000}],[40,{"fee":2000000}],[41,{"fee":2000000}]],"scale":10000},"block_interval":5,"maintenance_interval":86400,"maintenance_skip_slots":3,"committee_proposal_review_period":1209600,"maximum_transaction_size":2048,"maximum_block_size":2000000,"maximum_time_until_expiration":86400,"maximum_proposal_lifetime":2419200,"maximum_asset_whitelist_authorities":10,"maximum_asset_feed_publishers":10,"maximum_witness_count":1001,"maximum_committee_count":1001,"maximum_authority_membership":10,"reserve_percent_of_fee":2000,"network_percent_of_fee":2000,"lifetime_referrer_percent_of_fee":3000,"cashback_vesting_period_seconds":31536000,"cashback_vesting_threshold":10000000,"count_non_member_votes":true,"allow_non_member_whitelists":false,"witness_pay_per_block":1000000,"worker_budget_per_day":"50000000000","max_predicate_opcode":1,"fee_liquidation_threshold":10000000,"accounts_per_fee_scale":1000,"account_fee_scale_bitshifts":4,"max_authority_depth":2,"extensions":[]},"next_available_vote_id":22,"active_committee_members":["1.4.0","1.4.1","1.4.2","1.4.3","1.4.4","1.4.5","1.4.6","1.4.7","1.4.8","1.4.9","1.4.10"],"active_witnesses":["1.5.1","1.5.2","1.5.3","1.5.4","1.5.5","1.5.6","1.5.7","1.5.8","1.5.9","1.5.10","1.5.11"]}}



request[7]: {"id":7,"method":"call","params":[2, "get_dynamic_global_properties", []]}

return: (len=501): {"id":7,"jsonrpc":"2.0","result":{"id":"2.1.0","head_block_number":424812,"head_block_id":"00067b6c0bb08b1171dc78f53b2168e591b4f313","time":"2018-12-04T09:20:30","current_witness":"1.5.3","next_maintenance_time":"2018-12-05T00:00:00","last_budget_time":"2018-12-04T00:00:00","witness_budget":0,"accounts_registered_this_interval":0,"recently_missed_count":0,"current_aslot":551751,"recent_slots_filled":"276126301261205709548220502031364751087","dynamic_flags":0,"last_irreversible_block_num":424803}}



request[8]: {"id":8,"method":"call","params":[2, "lookup_account_names", [["nathan"]]]}

return: (len=1164): {"id":8,"jsonrpc":"2.0","result":[{"id":"1.2.17","membership_expiration_date":"1969-12-31T23:59:59","registrar":"1.2.17","referrer":"1.2.17","lifetime_referrer":"1.2.17","network_fee_percentage":2000,"lifetime_referrer_fee_percentage":8000,"referrer_rewards_percentage":0,"name":"nathan","vm_type":"","vm_version":"","code":"","code_version":"","abi":{"version":"gxc::abi/1.0","types":[],"structs":[],"actions":[],"tables":[],"error_messages":[],"abi_extensions":[]},"owner":{"weight_threshold":1,"account_auths":[],"key_auths":[["BTS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",1]],"address_auths":[]},"active":{"weight_threshold":1,"account_auths":[],"key_auths":[["BTS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",1]],"address_auths":[]},"options":{"memo_key":"BTS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV","voting_account":"1.2.5","num_witness":0,"num_committee":0,"votes":[],"extensions":[]},"statistics":"2.6.17","whitelisting_accounts":[],"blacklisting_accounts":[],"whitelisted_accounts":[],"blacklisted_accounts":[],"cashback_vb":"1.10.11","owner_special_authority":[0,{}],"active_special_authority":[0,{}],"top_n_control_flags":0}]}



request[9]: {"id":9,"method":"call","params":[2, "get_dynamic_global_properties", []]}

return: (len=501): {"id":9,"jsonrpc":"2.0","result":{"id":"2.1.0","head_block_number":424812,"head_block_id":"00067b6c0bb08b1171dc78f53b2168e591b4f313","time":"2018-12-04T09:20:30","current_witness":"1.5.3","next_maintenance_time":"2018-12-05T00:00:00","last_budget_time":"2018-12-04T00:00:00","witness_budget":0,"accounts_registered_this_interval":0,"recently_missed_count":0,"current_aslot":551751,"recent_slots_filled":"276126301261205709548220502031364751087","dynamic_flags":0,"last_irreversible_block_num":424803}}

