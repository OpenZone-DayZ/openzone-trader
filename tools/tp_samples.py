"""Sample TraderPlus files for the stand's import check (design §11.3).

Writes $profile:TPImportTest (E:\\dayzmod\\testserver\\profiles\\TPImportTest) in
TraderPlus's own layout. The bank account is copied from the stand's real
TraderPlus bank folder with its licences replaced; its name holds a Steam id,
which this script never prints.
"""
import glob
import io
import json
import os

PROFILE = r"E:\dayzmod\testserver\profiles"
ROOT = os.path.join(PROFILE, "TPImportTest")


def put(rel, data):
    path = os.path.join(ROOT, rel)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with io.open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=4, ensure_ascii=False)


general = {
    "Version": "",
    "LicenceKeyWord": "Licence",
    "Licences": ["Car Licence", "Weapon Licence"],
    # TraderPlus writes its bools as 0/1.
    "AcceptedStates": {
        "AcceptWorn": 1, "AcceptDamaged": 1, "AcceptBadlyDamaged": 0,
        "CoefficientWorn": 0.75, "CoefficientDamaged": 0.5, "CoefficientBadlyDamaged": 0.25,
    },
    "StoreOnlyToPristineState": 0,
    "Currencies": [
        {"ClassName": "OZT_ProbeNote500", "Value": 500},
        {"ClassName": "TraderPlus_Money_Dollar100,OZT_ProbeNote100", "Value": 100},
        {"ClassName": "OZT_ProbeNote50,SomeoneElse_Coin", "Value": 50},
        {"ClassName": "OZT_ProbeNote20", "Value": 20},
        {"ClassName": "OZT_ProbeNote10", "Value": 10},
        {"ClassName": "OZT_ProbeNote", "Value": 5},
    ],
    "Traders": [
        {"Id": 1, "Name": "SurvivorM_Mirek", "GivenName": "Sidorovich", "Role": "General Trader",
         "Position": [4687.0, 339.0, 10195.5], "Orientation": [180.0, 0.0, 0.0],
         "Clothes": ["TShirt_Red", "Jeans_Blue", "AthleticShoes_Black"]},
        {"Id": 2, "Name": "Land_City_Stand_Grocery", "GivenName": "Kevin", "Role": "Weapon Trader",
         "Position": [4690.0, 340.77, 10198.0], "Orientation": [0.0, 0.0, 0.0], "Clothes": []},
        {"Id": 1, "Name": "TraderPlusSign", "GivenName": "", "Role": "Food Trader",
         "Position": [4695.0, 339.0, 10192.0], "Orientation": [90.0, 0.0, 0.0], "Clothes": []},
    ],
    "TraderObjects": [
        {"ObjectName": "Land_Misc_Toilet_Mobile", "Position": [4700.0, 339.0, 10200.0], "Orientation": [0.0, 0.0, 0.0]},
    ],
}

prices = {
    "Version": "",
    "EnableAutoDestockAtRestart": 0,
    "EnableDefaultTraderStock": 0,
    "TraderCategories": [
        {"CategoryName": "Food", "Products": [
            "TunaCan,1,-1,1,40,0.5",
            "Apple,0.95,10,1,10,5",
            "SodaCan_Cola,1,0,1,15,8",
            "SomeoneElse_Item,1,-1,1,10,5",
        ]},
        {"CategoryName": "Ammo", "Products": [
            "Ammo_762x39,0.95,10,20,100,50",
            "Ammo_9x19,1,-1,0.5,50,25",
            "TunaCan,1,-1,1,99,50",
        ]},
        {"CategoryName": "Weapons", "Products": [
            "AKM,0.95,3,1,3000,1500,0.25",
            "Mag_AKM_30Rnd,1,-1,0,300,150",
            "UniversalLight,1,5,1,200,0.5",
        ]},
        {"CategoryName": "Car Licence", "Products": [
            "Hatchback_02,1,-1,1,10000,5000",
            "CanisterGasoline,1,-1,1,100,50",
        ]},
        {"CategoryName": "Documents", "Products": [
            "Weapon Licence,1,-1,1,5000,-1",
            "Car Licence,1,-1,1,8000,-1",
        ]},
    ],
}

ids = {
    "Version": "",
    "IDs": [
        {"Id": 1, "Categories": ["Food", "Ammo", "Documents"], "LicencesRequired": [], "CurrenciesAccepted": []},
        {"Id": 2, "Categories": ["Weapons", "Ammo", "Car Licence"], "LicencesRequired": ["Weapon Licence"],
         "CurrenciesAccepted": ["TraderPlus_Money_Dollar100"]},
    ],
}

put(r"TraderPlusConfig\TraderPlusGeneralConfig.json", general)
put(r"TraderPlusConfig\TraderPlusPriceConfig.json", prices)
put(r"TraderPlusConfig\TraderPlusIDsConfig.json", ids)

# The stock, as TraderPlus keeps it: "class count level", the count in trades.
put(r"TraderPlusDatabase\ID_1\Food.json", {"CategoryName": "Food", "Products": ["Apple 4 0", "Apple 2 1", "TunaCan 5 0"]})
put(r"TraderPlusDatabase\ID_1\Ammo.json", {"CategoryName": "Ammo", "Products": ["ammo_762x39 3 0"]})
put(r"TraderPlusDatabase\ID_2\Weapons.json", {"CategoryName": "Weapons", "Products": ["AKM 2 0", "AKM 1 2", "UniversalLight 1 0"]})
put(r"TraderPlusDatabase\ID_2\Ammo.json", {"CategoryName": "Ammo", "Products": ["Ammo_762x39 1 0"]})

# The bank: the stand's own account, holding the weapon licence.
real = glob.glob(os.path.join(PROFILE, "TraderPlus", "TraderPlusBankDatabase", "Account_*.json"))
assert len(real) == 1, len(real)
with io.open(real[0], encoding="utf-8") as f:
    acc = json.load(f)
acc["Licences"] = ["Weapon Licence"]
put(os.path.join("TraderPlusBankDatabase", os.path.basename(real[0])), acc)
print("samples written; bank: one account, licences", acc["Licences"])
