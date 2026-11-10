# Documentation
Container Distribution Framework enables you to dynamically manipulate the inventory of non-actor references. In vanilla, references (world objects) to specific containers all share the same inventory "pool" - so, a chest in a dungeon can potentially have the same items in it as a chest in home, if they have the same **Base Form**.

CDF bypasses that limitation. After the reference has initialized its inventory, CDF adds, removes, and replaces items in it. For instance, you can make it so that food in vampire lairs becomes human viscera (hearts, flesh, blood, etc), or entirely remove food from Nordic Dungeons. You can make locked chests in dwemer dungeons have more gems, or you can even replace currency entirely - Much like C.O.I.N. does.

In order to achieve this, CDF uses "Configs" that are describe what should change and when. Configs are JSON files found within `Data/SKSE/Plugins/ContainerDistributionFramework`. 
## Rules
Each config can contain one or more `Rules`. A `Rule` is an instruction on how to modify a reference's inventory, and can is composed of tw parts:

- One or more `Changes`
- Optionally, one or more `Conditions`

A `Change` is the actual modification that will happen to the container. There are 5 types of `Changes`:

1. **Additions**
1. **Removals**
1. **Removals based on item Keywords**
1. **Replacements**
1. **Replacements based on item Keywords**

A `Condition` is something that describes *when* a change should be applied. Note that each change may have more than 1 conditions. There are a lot of condition types, and they are listed later.
## Change Applicaton Order
CDF applies rules in a deterministic order:

1. Additions
1. Removals
1. Removals (by keyword)
1. Replacements
1. Replacements (by keyword)

Note that this isn't per config - **ALL** additions from **EVERY** config will be processed **BEFORE** all removals. Take for instance these two configs:

```jsonc
// More_Treats.json
{
    "Changes": {
        "Add": [
            "JellybeansLeveledList",
            "SweetrollsLeveledList",
            "TaffyLeveledList"
        ],
        "Count": 2000
    }
}
```
For the purposes of this section, just trust me that this config adds *a ton* of candies everywhere.
```jsonc
// Foodless_Dungeons.json
{
    "Conditions": {
        "LocationKeywords": "LocTypeNordicDungeon"
    },
    "Changes": {
        "RemoveByKeyword": "VendorItemFood"
    }
}
```
This one removes all items that have the "Food" keyword, but only from Nordic dungeons

As you can imagine, if CDF followed SPID rules, `Foodless_Dungeons` would run before `More_Treats`. This would remove all food from everywhere... and then add treats. You *could* actually combat this by renaming the config files, but this isn't a realistic solution. Instead, when parsing finishes, each "rule" is sorted based on its type.

## Conditions
A `Condition` is a series of **restrictions** that can apply to one or more `Changes`. CDF's goal is not to be a Skypatcher or Container Item Distributor alternative, but actually address a limitation that most people run into when modding. Thus, you will almost always want a condition when making a config.

You have already seen what `Conditions` broadly look like above. They are split into two categories:

- Flags\
Examples: `AllowVendors`, `OnlyVendors`, `AllowNoRespawn`
- Complex\
Examples: `LocationKeywords`, `Worldspaces`, `BaseForm`

You can combine any number of `Conditions`, and as long as **all** of them are true the `Rule` will apply. Note that individual components can be true if only one of their requirements are met - for instance, `Worldspaces` only need 1 match even if multiple worldspaces are supplied but `LocationKeywords` need the location to have all keywords. There are ways to change this behavior, but more on that later.

## Chnges
A `Change` is a description of how to modify the container. Each rule can have one or more `Chages` specified, and they all share the same rules if any exist. Note that `Rules` **MUST** have at least one `Change`. If they don't, they are considered invalid. 

A `Change` can have up to three field types:
- Additions\
What to add to the container. This can be one thing, or multiple. Leveled lists are supported.
- Removals\
What to remove from the container. This can be one thing, multiple, formlists, or keywords.
- Count\
How many things to add/remove. Count behaves differently based on if you have just an Addition, just a Removal, or both. 

| Behavior When | Only Addition | Only Removal | Addition & Removal |
| :-- | :--: | :--: | :--: |
|Has Count | `Count` copies of each specified item are added to the container. | Up to `Count` of each matching item are removed from the contianer. | Count is ignored. |
|No Count | Only 1 copy is added to the container. | All matching items are removed from the container. | Count is ignored. | 