/* Copyright (C) 2006-2017 PUC-Rio/Laboratorio TeleMidia

This file is part of Ginga (Ginga-NCL).

Ginga is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Ginga is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
License for more details.

You should have received a copy of the GNU General Public License
along with Ginga.  If not, see <http://www.gnu.org/licenses/>.  */

#include "ginga.h"
#include "NclComponentsConverter.h"

GINGA_PRAGMA_DIAG_IGNORE (-Wsign-conversion)

GINGA_NCLCONV_BEGIN

NclComponentsConverter::NclComponentsConverter (
    DocumentParser *documentParser)
    : NclComponentsParser (documentParser)
{
}

void
NclComponentsConverter::addPortToContext (void *parentObject,
                                          void *childObject)
{
  if (((Entity *)parentObject)->instanceOf ("ContextNode"))
    {
      ((ContextNode *)parentObject)->addPort ((Port *)childObject);
    }
}

void
NclComponentsConverter::addPropertyToContext (void *parentObject,
                                              void *childObject)
{
  if (((Entity *)parentObject)->instanceOf ("ContextNode"))
    {
      ((ContextNode *)parentObject)->addAnchor ((Anchor *)childObject);
    }
  else if (((Entity *)parentObject)->instanceOf ("ReferNode"))
    {
      ((ReferNode *)parentObject)->addAnchor ((Anchor *)childObject);
    }
}

void
NclComponentsConverter::addContextToContext (void *parentObject,
                                             void *childObject)
{
  if (((Entity *)parentObject)->instanceOf ("ContextNode"))
    {
      // adicionar composicao aa composicao
      addNodeToContext ((ContextNode *)parentObject,
                        (NodeEntity *)childObject);
    }
}

void
NclComponentsConverter::addSwitchToContext (void *parentObject,
                                            void *childObject)
{
  if (((Entity *)parentObject)->instanceOf ("ContextNode"))
    {
      // adicionar switch aa composicao
      addNodeToContext ((ContextNode *)parentObject,
                        (NodeEntity *)childObject);
    }
}

void
NclComponentsConverter::addMediaToContext (void *parentObject,
                                           void *childObject)
{
  if (((Entity *)parentObject)->instanceOf ("ContextNode"))
    {
      // adicionar media aa composicao
      addNodeToContext ((ContextNode *)parentObject,
                        (NodeEntity *)childObject);
    }
}

void
NclComponentsConverter::addLinkToContext (void *parentObject,
                                          void *childObject)
{
  int min;
  int max;
  Role *role;
  vector<Role *>::iterator i;

  if (((Entity *)parentObject)->instanceOf ("ContextNode"))
    {
      vector<Role *> *roles;

      roles = ((Link *)childObject)->getConnector ()->getRoles ();
      if (roles != NULL)
        {
          i = roles->begin ();
          while (i != roles->end ())
            {
              role = *i;
              // if (role->instanteOf("CardinalityRole") {
              min = role->getMinCon ();
              max = role->getMaxCon ();

              /*} else {
                      min = 1;
                      max = 1;
              }*/

              if (((Link *)childObject)->getNumRoleBinds (role)
                  < (unsigned int)min)
                {
                  clog << "NclComponentsConverter::addLinkToContext";
                  clog << " role " << role->getLabel ().c_str ();
                  clog << " with less than minimum binds." << endl;

                  delete ((Link *)childObject);
                  delete roles;

                  return;
                }
              else if (max > 0
                       && ((Link *)childObject)->getNumRoleBinds (role)
                              > (unsigned int)max)
                {
                  clog << "NclComponentsConverter::addLinkToContext";
                  clog << " role " << role->getLabel ().c_str ();
                  clog << " with more than maximum binds." << endl;

                  delete ((Link *)childObject);
                  delete roles;

                  return;
                }
              ++i;
            }
          delete roles;
        }

      ((ContextNode *)parentObject)->addLink ((Link *)childObject);
    }
}

void
NclComponentsConverter::addNodeToContext (ContextNode *contextNode,
                                          NodeEntity *node)
{
  // adicionar um noh aa composicao
  contextNode->addNode (node);
}

void
NclComponentsConverter::addAnchorToMedia (ContentNode *contentNode,
                                          Anchor *anchor)
{
  if (contentNode->getAnchor (anchor->getId ()) != NULL)
    {
      clog << "NclComponentsConverter::addAnchorToMedia ";
      clog << "There is another interface (area, attribute, etc.)";
      clog << " with the same id (" << anchor->getId ().c_str ();
      clog << ") defined for the" << contentNode->getId ().c_str ();
      clog << " media element." << endl;
    }
  else
    {
      contentNode->addAnchor (anchor);
    }
}

void
NclComponentsConverter::addAreaToMedia (void *parentObject,
                                        void *childObject)
{
  addAnchorToMedia ((ContentNode *)parentObject, (Anchor *)childObject);
}

void
NclComponentsConverter::addPropertyToMedia (void *parentObject,
                                            void *childObject)
{
  addAnchorToMedia ((ContentNode *)parentObject, (Anchor *)childObject);
}

void *
NclComponentsConverter::createContext (DOMElement *parentElement,
                                       arg_unused (void *objGrandParent))
{
  NclDocument *document;
  string id, attValue;
  Node *node;
  Entity *referNode;
  ContextNode *context;
  GenericDescriptor *descriptor;

  if (!parentElement->hasAttribute (XMLString::transcode ("id")))
    {
      clog << "A context element was declared";
      clog << " without an id attribute." << endl;
      return NULL;
    }

  id = XMLString::transcode (
      parentElement->getAttribute (XMLString::transcode ("id")));

  node = ((NclDocumentConverter *)getDocumentParser ())->getNode (id);
  if (node != NULL)
    {
      clog << "There is another node element previously declared with";
      clog << " the same " << id.c_str () << " id." << endl;
      return NULL;
    }

  if (parentElement->hasAttribute (XMLString::transcode ("refer")))
    {
      attValue = XMLString::transcode (
          parentElement->getAttribute (XMLString::transcode ("refer")));

      try
        {
          referNode = (ContextNode *)((NclDocumentConverter *)
                                          getDocumentParser ())
                          ->getNode (attValue);

          if (referNode == NULL)
            {
              document = (NclDocument *)(getDocumentParser ()->getObject (
                  "return", "document"));

              referNode = (ContextNode *)(document->getNode (attValue));
              if (referNode == NULL)
                {
                  referNode = (Entity *)(new ReferredNode (
                      attValue, (void *)parentElement));
                }
            }
        }
      catch (...)
        {
          clog << "The context element refers to " << attValue.c_str ();
          clog << " object, which is not a context element." << endl;
          return NULL;
        }

      node = new ReferNode (id);
      ((ReferNode *)node)->setReferredEntity (referNode);

      //((NclDocumentConverter*)getDocumentParser())->
      // addNode((NodeEntity*)node);

      return node;
    }

  // retornar nova composicao ncm a partir do elemento xml que a
  // representa em NCL
  context = new ContextNode (id);

  if (parentElement->hasAttribute (XMLString::transcode ("descriptor")))
    {
      // adicionar um descritor a um objeto de midia
      attValue = XMLString::transcode (parentElement->getAttribute (
          XMLString::transcode ("descriptor")));

      document = (NclDocument *)(getDocumentParser ()->getObject (
          "return", "document"));

      descriptor = document->getDescriptor (attValue);
      if (descriptor != NULL)
        {
          context->setDescriptor (descriptor);
        }
      else
        {
          clog << "The context element with " << id.c_str ();
          clog << " id attribute refers to a descriptor (";
          clog << attValue.c_str () << ") that"
               << " does not exist.";
          clog << endl;
          delete context;
          return NULL;
        }
    }

  return context;
}

void *
NclComponentsConverter::posCompileContext (DOMElement *parentElement,
                                           void *parentObject)
{
  clog << "posCompileContext" << endl;
  DOMNodeList *elementNodeList;
  int i, size;
  DOMNode *node;
  DOMElement *element;
  string elementTagName;
  void *elementObject;

  elementNodeList = parentElement->getChildNodes ();
  size = (int) elementNodeList->getLength ();

  for (i = 0; i < size; i++)
    {
      node = elementNodeList->item (i);
      if (node->getNodeType () == DOMNode::ELEMENT_NODE)
        {
          element = (DOMElement *)node;
          elementTagName = XMLString::transcode (element->getTagName ());
          if (XMLString::compareIString (elementTagName.c_str (), "context")
              == 0)
            {
              if (parentObject != NULL)
                {
                  elementObject
                      = ((ContextNode *)parentObject)
                            ->getNode (XMLString::transcode (
                                element->getAttribute (
                                    XMLString::transcode ("id"))));

                  /*
                   * This is an old version to get nested contexts.
                   * Nested contexts' ids are not in
                   * documentParser's genericTable.
                  elementObject = ((NclDocumentConverter*)
                                  getDocumentParser())->getNode(
                                                  XMLString::transcode(
                                                                  element->getAttribute(
                                                                                  XMLString::transcode(
                                                                                                  "id"))));
                  */

                  try
                    {
                      if (((NodeEntity *)elementObject)
                              ->instanceOf ("ContextNode"))
                        {
                          posCompileContext (element, elementObject);
                        }
                    }
                  catch (...)
                    {
                      // treating error with the <NodeEntity*> casting
                    }
                }
            }
          else if (XMLString::compareIString (elementTagName.c_str (),
                                              "switch")
                   == 0)
            {
              elementObject = ((NclDocumentConverter *)getDocumentParser ())
                                  ->getNode (XMLString::transcode (
                                      element->getAttribute (
                                          XMLString::transcode ("id"))));

              if (elementObject == NULL)
                {
                  clog << "NclComponentsConverter::posCompileContext ";
                  clog << "Error can't find '";
                  clog << XMLString::transcode (
                      element->getAttribute (XMLString::transcode ("id")));

                  clog << "' (switch)";
                  clog << endl;
                }
              else if (((NodeEntity *)elementObject)
                           ->instanceOf ("SwitchNode"))
                {
                  getPresentationControlParser ()->posCompileSwitch (
                      element, elementObject);
                }
            }
        }
    }

  return NclComponentsParser::posCompileContext (parentElement,
                                                 parentObject);
}

void *
NclComponentsConverter::createMedia (DOMElement *parentElement,
                                     arg_unused (void *objGrandParent))
{
  string attValue, id;
  NclDocument *document;
  Node *node;
  Entity *referNode;
  GenericDescriptor *descriptor;

  if (!parentElement->hasAttribute (XMLString::transcode ("id")))
    {
      clog << "Error: a media element was declared without an id";
      clog << " attribute." << endl;
      return NULL;
    }

  id = XMLString::transcode (
      parentElement->getAttribute (XMLString::transcode ("id")));

  node = ((NclDocumentConverter *)getDocumentParser ())->getNode (id);
  if (node != NULL)
    {
      clog << "There is another node element previously declared";
      clog << " with the same " << id.c_str () << " id." << endl;
      return NULL;
    }

  if (parentElement->hasAttribute (XMLString::transcode ("refer")))
    {
      attValue = XMLString::transcode (
          parentElement->getAttribute (XMLString::transcode ("refer")));

      try
        {
          referNode = (ContentNode *)((NclDocumentConverter *)
                                          getDocumentParser ())
                          ->getNode (attValue);

          if (referNode == NULL)
            {
              // TODO: verificar se faz a mesma coisa da linha anterior
              document = (NclDocument *)(getDocumentParser ()->getObject (
                  "return", "document"));

              referNode = (ContentNode *)document->getNode (attValue);
              if (referNode == NULL)
                {
                  referNode
                      = new ReferredNode (attValue, (void *)parentElement);
                }
            }
        }
      catch (...)
        {
          clog << "The media element refers to ";
          clog << attValue.c_str ();
          clog << " object, which is not a media element." << endl;
          return NULL;
        }

      node = new ReferNode (id);
      if (parentElement->hasAttribute (XMLString::transcode ("instance")))
        {
          attValue = XMLString::transcode (parentElement->getAttribute (
              XMLString::transcode ("instance")));

          ((ReferNode *)node)->setInstanceType (attValue);
        }
      ((ReferNode *)node)->setReferredEntity (referNode);

      return node;
    }

  node = new ContentNode (id, NULL, "");

  // type of media object
  if (parentElement->hasAttribute (XMLString::transcode ("type")))
    {
      ((ContentNode *)node)
          ->setNodeType (XMLString::transcode (
              parentElement->getAttribute (XMLString::transcode ("type"))));
    }

  if (parentElement->hasAttribute (XMLString::transcode ("src")))
    {
      string src = XMLString::transcode (
          parentElement->getAttribute (XMLString::transcode ("src")));

      if (src != "")
        {
          if (getDocumentParser ()->isAbsolutePath (src))
            {
              ((ContentNode *)node)
                  ->setContent (new AbsoluteReferenceContent (src));
            }
          else
            {
              ((ContentNode *)node)
                  ->setContent (new AbsoluteReferenceContent (
                      getDocumentParser ()->getDocumentPath () +
                      /*getDocumentParser()->getIUriD() +*/
                      src));
            }
        }
    }

  // testar duracao implicita do no' de midia
  /*
   if (parentElement.hasAttribute("implicitDur")) {
   String durStr = parentElement.getAttribute("implicitDur");
   durStr = durStr.substring(0, durStr.length() - 1);
   //converter duracao para double
   long dur = (long)((new Double(durStr)).doubleValue() * 1000);
   //recuperar ancora lambda do no' de midia
   ILambdaAnchor anchor = (ILambdaAnchor)node.getAnchor(0);
   //cast
   IIntervalRegion region = (IIntervalRegion)anchor.getRegion();
   //atribuir duracao implicita
   region.setEnd(dur);
   }
   */

  if (parentElement->hasAttribute (XMLString::transcode ("descriptor")))
    {
      // adicionar um descritor a um objeto de midia
      attValue = XMLString::transcode (parentElement->getAttribute (
          XMLString::transcode ("descriptor")));

      document = (NclDocument *)getDocumentParser ()->getObject (
          "return", "document");

      descriptor = document->getDescriptor (attValue);
      if (descriptor != NULL)
        {
          ((ContentNode *)node)->setDescriptor (descriptor);
        }
      else
        {
          clog << "The media element with " << id.c_str ();
          clog << " id attribute refers to a descriptor (";
          clog << attValue.c_str () << ") that does not exist." << endl;
          delete node;
          node = NULL;
          return NULL;
        }
    }

  //((NclDocumentConverter*)getDocumentParser())->addNode((NodeEntity*)node);

  // retornar no' de midia
  return node;
}

GINGA_NCLCONV_END
