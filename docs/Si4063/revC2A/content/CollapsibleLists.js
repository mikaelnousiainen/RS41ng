/*

CollapsibleLists.js

An object allowing lists to dynamically expand and collapse

Created by Stephen Morley - http://code.stephenmorley.org/ - and released under
the terms of the CC0 1.0 Universal legal code:

http://creativecommons.org/publicdomain/zero/1.0/legalcode

*/

// create the CollapsibleLists object
var CollapsibleLists =
    new function(){

      /* Makes all lists with the class 'collapsibleList' collapsible. The
       * parameter is:
       *
       * doNotRecurse - true if sub-lists should not be made collapsible
       */
      this.apply = function(doNotRecurse){

        // loop over the unordered lists
        var uls = document.getElementsByTagName('ul');
        for (var index = 0; index < uls.length; index ++){

          // check whether this list should be made collapsible
          if (uls[index].className.match(/(^| )collapsibleList( |$)/)){

            // make this list collapsible
            this.applyTo(uls[index], true);

            // check whether sub-lists should also be made collapsible
            if (!doNotRecurse){

              // add the collapsibleList class to the sub-lists
              var subUls = uls[index].getElementsByTagName('ul');
              for (var subIndex = 0; subIndex < subUls.length; subIndex ++){
                subUls[subIndex].className += ' collapsibleList';
              }

            }

          }

        }

      };

      /* Makes the specified list collapsible. The parameters are:
       *
       * node         - the list element
       * doNotRecurse - true if sub-lists should not be made collapsible
       */
      this.applyTo = function(node, doNotRecurse){

        // loop over the list items within this node
        var lis = node.getElementsByTagName('li');
        for (var index = 0; index < lis.length; index ++){

          // check whether this list item should be collapsible
          if (!doNotRecurse || node == lis[index].parentNode){

            if (lis[index].title != 'enumeration'){
              
              // prevent text from being selected unintentionally
              if (lis[index].addEventListener){
                lis[index].addEventListener(
                    'mousedown', function (e){ e.preventDefault(); }, false);
              }else{
                lis[index].attachEvent(
                    'onselectstart', function(){ event.returnValue = false; });
              }
              
              // add the click listener
              if (lis[index].addEventListener){
                lis[index].addEventListener(
                  'click', createClickListener(lis[index]), false);
              }else{
                lis[index].attachEvent(
                  'onclick', createClickListener(lis[index]));
              }
              // close the unordered lists within this list item
              toggle(lis[index]);
            }

          }

        }

      };

      /* Returns a function that toggles the display status of any unordered
       * list elements within the specified node. The parameter is:
       *
       * node - the node containing the unordered list elements
       */
      function createClickListener(node){

        // return the function
        return function(e){

          // ensure the event object is defined
          if (!e) e = window.event;

          // find the list item containing the target of the event
          var li = (e.target ? e.target : e.srcElement);
          while (li.nodeName != 'LI') li = li.parentNode;

          // toggle the state of the node if it was the target of the event
          if (li == node) toggle(node);

        };

      }

      /* Expand all ul with collapsibleList className for searching
       *
       */
      this.expandAll = function(){
      

        // loop over the unordered lists
        var uls = document.getElementsByTagName('ul');
        for (var index = 0; index < uls.length; index ++){

          // check whether this list should be made collapsible
          if (uls[index].className.match(/(^| )collapsibleList( |$)/)){

            // expand the list
            this.expandList(uls[index]);
          }

        }
      };

      /* Expand all items in list.
       */
      this.expandList = function(node) {
        
        // loop over the list items within this node
        var lis = node.getElementsByTagName('li');
        for (var index = 0; index < lis.length; index ++){

          this.expandListItem(lis[index]);
        }
      };

      /* Expand list item.
       */
      this.expandListItem = function(node) {

        // determine whether to open or close the unordered lists
        var open = node.className.match(/(^| )collapsibleListClosed( |$)/);
        
        if (open)
            toggle(node);

      };

      /* Collapse all ul with collapsibleList className for searching
       *
       */
      this.collapseAll = function(){
      
        // loop over the unordered lists
        var uls = document.getElementsByTagName('ul');
        for (var index = 0; index < uls.length; index ++){

          // check whether this list should be made collapsible
          if (uls[index].className.match(/(^| )collapsibleList( |$)/)){

            // collapse the list
            this.collapseList(uls[index]);
          }

        }
      };

      /* Expand all items in list.
       */
      this.collapseList = function(node) {
        
        // loop over the list items within this node
        var lis = node.getElementsByTagName('li');
        for (var index = 0; index < lis.length; index ++){

          collapseListItem(lis[index]);
        }
      };

      /* Collapse list item.
       */
      function collapseListItem(node){

        // determine whether to open or close the unordered lists
        var open = node.className.match(/(^| )collapsibleListClosed( |$)/);
        
        if (!open)
            toggle(node);

      }
      /* Opens or closes the unordered list elements directly within the
       * specified node. The parameter is:
       *
       * node - the node containing the unordered list elements
       */
      function toggle(node){

        // determine whether to open or close the unordered lists
        var open = node.className.match(/(^| )collapsibleListClosed( |$)/);

        // loop over the unordered list elements with the node
        var uls = node.getElementsByTagName('ul');
        for (var index = 0; index < uls.length; index ++){

          // find the parent list item of this unordered list
          var li = uls[index];
          while (li.nodeName != 'LI') li = li.parentNode;

          // style the unordered list if it is directly within this node
          if (li == node) uls[index].style.display = (open ? 'block' : 'none');

        }

        // remove the current class from the node
        node.className =
            node.className.replace(
                /(^| )collapsibleList(Open|Closed)( |$)/, '');

        // if the node contains unordered lists, set its class
        if (uls.length > 0){
          node.className += ' collapsibleList' + (open ? 'Open' : 'Closed');
        }

      }

    }();
