
    function getItem(id)
    {
        var itm = false;
        if(document.getElementById)
            itm = document.getElementById(id);
        else if(document.all)
            itm = document.all[id];
        else if(document.layers)
            itm = document.layers[id];

        return itm;
    }

    function toggleItems(id_expand, id_collapse) 
    {
        var itm_expand = getItem(id_expand);
        var itm_collapse = getItem(id_collapse);

        if ((!itm_expand) || (!itm_collapse)) {
            return false;
        }

        if(itm_expand.style.display == 'none') {
            itm_expand.style.display = '';
            itm_collapse.style.display = 'none';
        }
        else {
            itm_expand.style.display = 'none';
            itm_collapse.style.display = '';
        }
    }

    function sum_cmd_set_show(cmd_set_key) 
    {
        var item_names = CMD_SET_KEYS[cmd_set_key];

        for (var i in item_names)
        {
            var item = getItem(item_names[i]);
            if (!item) {
                return false;
            }
            item.style.display = '';
        }
    }

    function sum_cmd_set_hide(cmd_set_key) 
    {
        var item_names = CMD_SET_KEYS[cmd_set_key];

        for (var i in item_names)
        {
            var item = getItem(item_names[i]);
            if (!item) {
                return false;
            }
            item.style.display = 'none';
        }
    }

    function sum_prop_grp_show_dflt(prop_grp_key) 
    {
        var show_item_names = PROP_GRP_KEYS[prop_grp_key]['showItems'];
        var hide_item_names = PROP_GRP_KEYS[prop_grp_key]['hideItems'];

        for (var i in show_item_names)
        {
            var item = getItem(show_item_names[i]);
            if (!item) {
                return false;
            }
            item.style.display = '';
        }
        for (var i in hide_item_names)
        {
            var item = getItem(hide_item_names[i]);
            if (!item) {
                return false;
            }
            item.style.display = 'none';
        }
    }
    
    function sum_prop_grp_show_all(prop_grp_key) 
    {
        var show_item_names = PROP_GRP_KEYS[prop_grp_key]['showAllItems'];
        var hide_item_names = PROP_GRP_KEYS[prop_grp_key]['hideAllItems'];

        for (var i in show_item_names)
        {
            var item = getItem(show_item_names[i]);
            if (!item) {
                return false;
            }
            item.style.display = '';
        }
        for (var i in hide_item_names)
        {
            var item = getItem(hide_item_names[i]);
            if (!item) {
                return false;
            }
            item.style.display = 'none';
        }
    }

    function sum_prop_grp_hide(prop_grp_key) 
    {
        var all_item_names = [];
        all_item_names = all_item_names.concat(PROP_GRP_KEYS[prop_grp_key]['showItems']);
        all_item_names = all_item_names.concat(PROP_GRP_KEYS[prop_grp_key]['hideItems']);

        for (var i in all_item_names) {
            var item = getItem(all_item_names[i]);
            if (!item) {
                return false;
            }
            item.style.display = 'none';
        }
    }

    function toggle_sum_prop_grp(prop_grp_key_suffix)
    {
        grp_hdr = getItem('sum_prop_grp_hdr_' + prop_grp_key_suffix);

        if (grp_hdr.style.display == 'none')
        {
            sum_prop_grp_show_dflt('key_' + prop_grp_key_suffix);
        } else {
            sum_prop_grp_hide('key_' + prop_grp_key_suffix);
        }
    }

    function toggle_sum_cmd_set(cmd_set_key_suffix)
    {
        cmd_set_hdr = getItem('sum_cmd_set_hdr_' + cmd_set_key_suffix);

        if (cmd_set_hdr.style.display == 'none')
        {
            sum_cmd_set_show('key_' + cmd_set_key_suffix);
        } else {
            sum_cmd_set_hide('key_' + cmd_set_key_suffix);
        }

    }

    function startup() {
        CollapsibleLists.apply();
    }

    function global_expand_all_tables() {
        
        // loop over the img elements
        var imgs = document.getElementsByTagName('img');
        for (var index = 0; index < imgs.length; index ++)
        {
            // Command Set Header
            
            // hide cmd set plus image
            if (imgs[index].className.match(/(^| )cmd_set_show( |$)/))
            {
                imgs[index].style.display = 'none';
            }
            
            // show cmd set minus image
            else if (imgs[index].className.match(/(^| )cmd_set_hide( |$)/))
            {
                imgs[index].style.display = '';
            }
            
            // Property Group Header
            
            // hide prop group plus image
            else if (imgs[index].className.match(/(^| )prop_grp_show( |$)/))
            {
                imgs[index].style.display = 'none';
            }
            
            // show prop group minus image
            else if (imgs[index].className.match(/(^| )prop_grp_hide( |$)/))
            {
                imgs[index].style.display = '';
            }

            // Command Fields
            
            // hide cmd field plus image
            else if (imgs[index].className.match(/(^| )cmd_field_expand( |$)/))
            {
                imgs[index].style.display = 'none';
            }
            
            // show cmd field minus image
            else if (imgs[index].className.match(/(^| )cmd_field_collapse( |$)/))
            {
                imgs[index].style.display = '';
            }
            
            // Properties Fields
            
            // hide prop field plus image
            else if (imgs[index].className.match(/(^| )prop_field_expand( |$)/))
            {
                imgs[index].style.display = 'none';
            }
            
            // show prop field minus image
            else if (imgs[index].className.match(/(^| )prop_field_collapse( |$)/))
            {
                imgs[index].style.display = '';
            }
            
        }
        CollapsibleLists.expandAll();
        for (var key in PROP_GRP_KEYS) {
            sum_prop_grp_show_all(key);
        }
        for (var key in CMD_SET_KEYS) {
            sum_cmd_set_show(key);
        }
    }
    
    function global_collapse_all_tables() {
    
        // loop over the img elements
        var imgs = document.getElementsByTagName('img');
        for (var index = 0; index < imgs.length; index ++)
        {

            // Command Set Header
            
            // show cmd set plus image
            if (imgs[index].className.match(/(^| )cmd_set_show( |$)/))
            {
                imgs[index].style.display = '';
            }

            // hide cmd set minus image
            else if (imgs[index].className.match(/(^| )cmd_set_hide( |$)/))
            {
                imgs[index].style.display = 'none';
            }
            
            
            // Property Group Header
            
            // show prop group plus image
            else if (imgs[index].className.match(/(^| )prop_grp_show( |$)/))
            {
                imgs[index].style.display = '';
            }
            
            // hide prop group minus image
            else if (imgs[index].className.match(/(^| )prop_grp_hide( |$)/))
            {
                imgs[index].style.display = 'none';
            }
            

            // Command Fields
            
            // show cmd field minus image
            else if (imgs[index].className.match(/(^| )cmd_field_expand( |$)/))
            {
                imgs[index].style.display = '';
            }
            
            // hide cmd field plus image
            else if (imgs[index].className.match(/(^| )cmd_field_collapse( |$)/))
            {
                imgs[index].style.display = 'none';
            }

            // Property Fields
            
            // show prop field minus image
            else if (imgs[index].className.match(/(^| )prop_field_expand( |$)/))
            {
                imgs[index].style.display = '';
            }
            
            // hide prop field plus image
            else if (imgs[index].className.match(/(^| )prop_field_collapse( |$)/))
            {
                imgs[index].style.display = 'none';
            }
            
            
        }
        CollapsibleLists.collapseAll();
        for (var key in PROP_GRP_KEYS) {
            sum_prop_grp_hide(key);
        }
        for (var key in CMD_SET_KEYS) {
            sum_cmd_set_hide(key);
        }
    }
