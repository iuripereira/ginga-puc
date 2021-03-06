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
#include "NclImportConverter.h"

GINGA_NCLCONV_BEGIN

NclImportConverter::NclImportConverter (DocumentParser *documentParser)
    : NclImportParser (documentParser)
{
}

void
NclImportConverter::addImportNCLToImportedDocumentBase (arg_unused (void *parentObject),
                                                        void *childObject)
{
  string docAlias, docLocation;
  NclDocumentConverter *compiler;
  NclDocument *thisDocument, *importedDocument;

  docAlias = XMLString::transcode (
      ((DOMElement *)childObject)
          ->getAttribute (XMLString::transcode ("alias")));

  docLocation = XMLString::transcode (
      ((DOMElement *)childObject)
          ->getAttribute (XMLString::transcode ("documentURI")));

  compiler = (NclDocumentConverter *)getDocumentParser ();
  importedDocument = compiler->importDocument (docLocation);
  if (importedDocument != NULL)
    {
      thisDocument = (NclDocument *)getDocumentParser ()->getObject (
          "return", "document");

      thisDocument->addDocument (importedDocument, docAlias, docLocation);
    }
}

void *
NclImportConverter::createImportBase (DOMElement *parentElement,
                                      arg_unused (void *objGrandParent))
{
  return parentElement;
}

void *
NclImportConverter::createImportNCL (DOMElement *parentElement,
                                     arg_unused (void *objGrandParent))
{
  return parentElement;
}
void *
NclImportConverter::createImportedDocumentBase (DOMElement *parentElement,
                                                arg_unused (void *objGrandParent))
{
  return parentElement;
}

GINGA_NCLCONV_END
