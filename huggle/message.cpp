//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include "message.hpp"

using namespace Huggle;

Message::Message(WikiUser *target, QString MessageText, QString MessageSummary)
{
    // we copy the user here so that we can let the caller delete it
    this->user = new WikiUser(target);
    this->Text = MessageText;
    this->Summary = MessageSummary;
    this->Suffix = true;
    this->Dependency = NULL;
    this->qToken = NULL;
    this->SectionKeep = false;
    this->query = NULL;
    this->_Status = Huggle::MessageStatus_None;
    this->PreviousTalkPageRetrieved = false;
    this->Page = "";
    this->TimeOfBase = "";
    this->Error = MessageError_NoError;
    this->ErrorText = "";
    this->Title = "Message from " + Configuration::HuggleConfiguration->UserName;
}

Message::~Message()
{
    if (this->query != NULL)
    {
        this->query->UnregisterConsumer(HUGGLECONSUMER_MESSAGE_SEND);
    }
    delete this->user;
}

void Message::RetrieveToken()
{
    this->_Status = Huggle::MessageStatus_RetrievingToken;
    this->qToken = new ApiQuery();
    this->qToken->SetAction(ActionQuery);
    this->qToken->Parameters = "prop=info&intoken=edit&titles=" + QUrl::toPercentEncoding(this->user->GetTalk());
    this->qToken->Target = Localizations::HuggleLocalizations->Localize("message-retrieve-new-token", this->user->GetTalk());
    this->qToken->RegisterConsumer(HUGGLECONSUMER_MESSAGE_SEND);
    Core::HuggleCore->AppendQuery(this->qToken);
    this->qToken->Process();
}

void Message::Send()
{
    if (this->HasValidEditToken())
    {
        this->PreflightCheck();
        return;
    }
    // first we need to get an edit token
    this->RetrieveToken();
}

void Message::Fail(QString reason)
{
    QStringList parameters;
    parameters << this->user->Username << reason;
    Huggle::Syslog::HuggleLogs->ErrorLog(Localizations::HuggleLocalizations->Localize("message-er", parameters));
    this->_Status = Huggle::MessageStatus_Failed;
    this->Error = Huggle::MessageError_Unknown;
    this->ErrorText = reason;
    if (this->qToken != NULL)
    {
        this->qToken->UnregisterConsumer(HUGGLECONSUMER_MESSAGE_SEND);
        this->qToken = NULL;
    }
    if (this->query != NULL)
    {
        this->query->UnregisterConsumer(HUGGLECONSUMER_MESSAGE_SEND);
        this->query = NULL;
    }
}

bool Message::IsFinished()
{
    if (this->Done())
    {
        return true;
    }
    if (this->Dependency != NULL)
    {
        if (!this->Dependency->IsProcessed())
        {
            return false;
        } else
        {
            if (this->Dependency->IsFailed())
            {
                // we can't continue because the dependency is fucked
                this->Dependency->UnregisterConsumer("keep");
                this->Dependency = NULL;
                this->_Status = Huggle::MessageStatus_Failed;
                this->Error = MessageError_Dependency;
                if (this->query != NULL)
                {
                    this->query->UnregisterConsumer(HUGGLECONSUMER_MESSAGE_SEND);
                    this->query = NULL;
                }
                return true;
            }
            // dependency finished OK so we can remove a pointer to it
            this->Dependency->UnregisterConsumer("keep");
            this->Dependency = NULL;
        }
    }
    if (this->RetrievingToken())
    {
        // we are retrieving token so we can check if the token
        // has beed retrieved and if yes, we can continue
        if (this->FinishToken())
        {
            // good, we have the token now so we can edit the page, however some checks are needed before we do that
            this->PreflightCheck();
        }
    }
    this->Finish();
    return this->Done();
}

bool Message::IsFailed()
{
    return this->_Status == Huggle::MessageStatus_Failed;
}

bool Message::HasValidEditToken()
{
    return (Configuration::HuggleConfiguration->SystemConfig_EditToken != "");
}

bool Message::RetrievingToken()
{
    return (this->_Status == Huggle::MessageStatus_RetrievingToken);
}

bool Message::IsSending()
{
    return (this->_Status == Huggle::MessageStatus_SendingMessage);
}

bool Message::Done()
{
    return (this->_Status == Huggle::MessageStatus_Done || this->_Status == Huggle::MessageStatus_Failed);
}

void Message::Finish()
{
    if (this->Done())
    {
        // we really need to quit now because query is null
        return;
    }
    if (!this->HasValidEditToken())
    {
        // we need to get a token
        if (this->query == NULL)
        {
            this->RetrieveToken();
            return;
        }
    }
    if (this->query == NULL)
    {
        // we should never reach this code
        return;
    }
    if (this->_Status == Huggle::MessageStatus_RetrievingTalkPage && !this->PreviousTalkPageRetrieved)
    {
        // we need to finish retrieving of previous talk page
        if (!this->query->IsProcessed())
        {
            return;
        }
        if (this->query->Result->Failed)
        {
            /// \todo LOCALIZE ME
            this->Fail("unable to retrieve the user talk page");
            return;
        }
        this->ProcessTalk();
        this->query->UnregisterConsumer(HUGGLECONSUMER_MESSAGE_SEND);
        this->query = NULL;
        // we should be able to finish sending now
        this->ProcessSend();
        return;
    }

    if (this->_Status == Huggle::MessageStatus_SendingMessage)
    {
        // we need to check the query
        if (!this->query->IsProcessed())
        {
            return;
        }

        if (this->query->IsFailed())
        {
            this->Fail("Failed to deliver the message");
            return;
        }

        bool sent = false;

        QDomDocument d;
        d.setContent(query->Result->Data);
        QDomNodeList e = d.elementsByTagName("error");
        if (e.count() > 0)
        {
            QDomElement element = e.at(0).toElement();
            if (element.attributes().contains("code"))
            {
                QString ec = element.attribute("code");
                if (ec == "editconflict")
                {
                    // someone edit the page meanwhile which means that our token has expired
                    this->Fail("Edit conflict");
                    this->Error = MessageError_Obsolete;
                } else
                {
                    this->Fail("Unknown error: " + ec);
                    this->Error = MessageError_Unknown;
                }
                return;
            }
        }
        QDomNodeList l = d.elementsByTagName("edit");
        if (l.count() > 0)
        {
            QDomElement element = l.at(0).toElement();
            if (element.attributes().contains("result"))
            {
                if (element.attribute("result") == "Success")
                {
                    /// \todo LOCALIZE ME
                    Huggle::Syslog::HuggleLogs->Log("Successfuly delivered message to " + this->user->Username);
                    sent = true;
                    HistoryItem item;
                    item.Result = "Success";
                    item.Type = HistoryMessage;
                    item.Target = user->Username;
                    if (Core::HuggleCore->Main != NULL)
                    {
                        Core::HuggleCore->Main->_History->Prepend(item);
                    }
                }
            }
        }

        if (!sent)
        {
            /// \todo LOCALIZE ME
            Huggle::Syslog::HuggleLogs->Log("Failed to deliver a message to " + this->user->Username + " please check logs");
            Huggle::Syslog::HuggleLogs->DebugLog(this->query->Result->Data);
        }

        this->query->UnregisterConsumer(HUGGLECONSUMER_MESSAGE_SEND);
        this->_Status = Huggle::MessageStatus_Done;
        this->query = NULL;
    }
}

bool Message::FinishToken()
{
    if (this->qToken == NULL)
    {
        throw new Huggle::Exception("qToken must not be NULL in this context", "void Message::FinishToken()");
    }
    if (!this->qToken->IsProcessed())
    {
        return false;
    }
    if (this->qToken->Result->Failed)
    {
        /// \todo LOCALIZE ME
        this->Fail("unable to retrieve the edit token");
        return false;
    }
    QDomDocument d;
    d.setContent(this->qToken->Result->Data);
    QDomNodeList l = d.elementsByTagName("page");
    if (l.count() == 0)
    {
        /// \todo LOCALIZE ME
        this->Fail("no token was returned by request");
        Huggle::Syslog::HuggleLogs->DebugLog("No page");
        return false;
    }
    QDomElement element = l.at(0).toElement();
    if (!element.attributes().contains("edittoken"))
    {
        /// \todo LOCALIZE ME
        this->Fail("the result doesn't contain the token");
        Huggle::Syslog::HuggleLogs->DebugLog("No token");
        return false;
    }
    Configuration::HuggleConfiguration->SystemConfig_EditToken = element.attribute("edittoken");
    this->qToken->UnregisterConsumer(HUGGLECONSUMER_MESSAGE_SEND);
    this->qToken = NULL;
    return true;
}

void Message::PreflightCheck()
{
    // check if we can directly append the data or if we need to fetch the previous page content before we do that
    if ((!this->Section || this->SectionKeep) && !this->PreviousTalkPageRetrieved)
    {
        if (this->query != NULL)
        {
            Syslog::HuggleLogs->DebugLog("Warning, Message::query != NULL on new allocation, possible memory leak in void Message::PreflightCheck()");
        }
        this->_Status = MessageStatus_RetrievingTalkPage;
        // we need to retrieve the talk page
        this->query = new ApiQuery();
        this->query->RegisterConsumer(HUGGLECONSUMER_MESSAGE_SEND);
        this->query->SetAction(ActionQuery);
        this->query->Parameters = "prop=revisions&rvprop=" + QUrl::toPercentEncoding("timestamp|user|comment|content") + "&titles=" +
                QUrl::toPercentEncoding(this->user->GetTalk());
        // inform user what is going on
        Core::HuggleCore->AppendQuery(this->query);
        /// \todo LOCALIZE ME
        this->query->Target = "Reading TP of " + this->user->Username;
        this->query->Process();
    } else
    {
        this->PreviousTalkPageRetrieved = true;
        this->ProcessSend();
    }
}

void Message::ProcessSend()
{
    this->_Status = MessageStatus_SendingMessage;
    if (this->query != NULL)
    {
        Syslog::HuggleLogs->DebugLog("Warning, Message::query != NULL on new allocation, possible memory leak in void Message::ProcessSend()");
    }
    this->query = new ApiQuery();
    // prevent message from being sent twice
    this->query->RetryOnTimeoutFailure = false;
    this->query->Timeout = 600;
    this->query->Target = "Writing " + this->user->GetTalk();
    this->query->UsingPOST = true;
    this->query->RegisterConsumer(HUGGLECONSUMER_MESSAGE_SEND);
    this->query->SetAction(ActionEdit);
    QString s = Summary;
    QString base = "";
    if (this->TimeOfBase != "")
    {
        base = "&basetimestamp=" + QUrl::toPercentEncoding(this->TimeOfBase);
    }
    if (this->Suffix)
    {
        s += " " + Configuration::HuggleConfiguration->EditSuffixOfHuggle;
    }
    if (this->SectionKeep || !this->Section)
    {
        if (this->SectionKeep)
        {
            this->Text = this->Append(this->Text, this->Page, this->Title);
        } else
        {
            // original page needs to be included in new value
            if (this->Page != "")
            {
                this->Text = this->Page + "\n\n" + this->Text;
            }
        }
        this->query->Parameters = "title=" + QUrl::toPercentEncoding(user->GetTalk()) + "&summary=" + QUrl::toPercentEncoding(s)
                + "&text=" + QUrl::toPercentEncoding(this->Text) + base
                + "&token=" + QUrl::toPercentEncoding(Configuration::HuggleConfiguration->SystemConfig_EditToken);
    }else
    {
        this->query->Parameters = "title=" + QUrl::toPercentEncoding(user->GetTalk()) + "&section=new&sectiontitle="
                + QUrl::toPercentEncoding(this->Title) + "&summary=" + QUrl::toPercentEncoding(s)
                + "&text=" + QUrl::toPercentEncoding(this->Text) + base + "&token="
                + QUrl::toPercentEncoding(Configuration::HuggleConfiguration->SystemConfig_EditToken);
    }
    Core::HuggleCore->AppendQuery(query);
    this->query->Process();
}

void Message::ProcessTalk()
{
    QDomDocument d;
    d.setContent(this->query->Result->Data);
    this->PreviousTalkPageRetrieved = true;
    QDomNodeList page = d.elementsByTagName("rev");
    QDomNodeList code = d.elementsByTagName("page");
    bool missing = false;
    if (code.count() > 0)
    {
        QDomElement e = code.at(0).toElement();
        if (e.attributes().contains("missing"))
        {
            missing = true;
        }
    }
    // get last id
    if (missing != true && page.count() > 0)
    {
        QDomElement e = page.at(0).toElement();
        if (e.nodeName() == "rev")
        {
            this->Page = e.text();
            this->PreviousTalkPageRetrieved = true;
            return;
        } else
        {
            /// \todo LOCALIZE ME
            this->Fail("Unable to retrieve " + this->user->GetTalk() + " stopping message delivery to that user");
            return;
        }
    } else
    {
        if (!missing)
        {
            /// \todo LOCALIZE ME
            this->Fail("Unable to retrieve " + this->user->GetTalk() + " stopping message delivery to that user");
            Huggle::Syslog::HuggleLogs->DebugLog(this->query->Result->Data);
            return;
        }
    }
}

QString Message::Append(QString text, QString OriginalText, QString Label)
{
    QRegExp regex("\\s*==\\s*" + QRegExp::escape(Label) + "\\s*==");
    if (!OriginalText.contains(regex))
    {
        // there is no section to append to
        if (OriginalText != "")
        {
            OriginalText = OriginalText + "\n\n";
        }
        OriginalText += "== " + Label + " ==\n\n" + text;
        return OriginalText;
    }
    QRegExp header("^\\s*==.*==\\s*$");
    int Post = Text.lastIndexOf(regex);
    // we need to check if there is any other section after this one
    QString Section = OriginalText.mid(Post);
    if (Section.contains("\n"))
    {
        // cut the header text
        int Diff = Section.indexOf("\n") + 1;
        Post += Diff;
        Section = Section.mid(Diff);
    }
    // we assume there is no other section and if there is some we change this
    int StartPoint = OriginalText.length();
    if (Section.contains(header))
    {
        // yes there is some other section, so we need to know where it is
        StartPoint = OriginalText.indexOf(header, Post);
    }
    // write the text exactly after the start point, but leave some space after it
    OriginalText = OriginalText.insert(StartPoint, "\n\n" + text + "\n\n");
    return OriginalText;
}
