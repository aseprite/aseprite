// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context.h"
#include "app/i18n/strings.h"
#include "app/ui/main_window.h"
#include "base/thread.h"
#include "feedback.xml.h"
#include "os/window.h"
#include "ui/manager.h"

#if ENABLE_SENTRY
  #include "app/sentry_wrapper.h"
#endif

#include "ui/system.h"

namespace app {

using namespace ui;

#if ENABLE_SENTRY
namespace {
std::string face_to_feedback_type(const int face)
{
  switch (face) {
    case 0:  return "positive";
    case 1:  return "neutral";
    case 2:  return "negative";
    default: return "";
  }
}

class FeedbackWindow final : public gen::Feedback {
public:
  explicit FeedbackWindow(const Sentry::Feedback& feedback)
  {
    faces()->ItemChange.connect(&FeedbackWindow::onFaceChange, this);

    email()->Change.connect([this] {
      // Very basic email validation
      const auto& text = email()->text();
      const size_t atPos = text.find('@');
      if (!text.empty() && (atPos == std::string::npos || atPos == text.length() - 1))
        submitButton()->setEnabled(false);
      else
        submitButton()->setEnabled(true);
    });

    submitButton()->Click.connect(&FeedbackWindow::onSubmit, this);

    if (feedback.type != face_to_feedback_type(faces()->selectedItem())) {
      int item = 1;
      if (feedback.type == "positive")
        item = 0;
      else if (feedback.type == "negative")
        item = 2;

      faces()->setSelectedItem(item);
      onFaceChange();
    }
    email()->setText(feedback.email);
    email()->Change(); // Ensure we validate it.
    comments()->setText(feedback.comments);

    centerWindow();
  }

  Sentry::Feedback feedback() const
  {
    if (m_submitted)
      return {};

    Sentry::Feedback f;
    f.email = email()->text();
    f.comments = comments()->text();
    f.type = face_to_feedback_type(faces()->selectedItem());
    f.attachExtras = attachExtras()->isSelected();
    return f;
  }

  void showLoading() const
  {
    faces()->setEnabled(false);
    email()->setEnabled(false);
    comments()->setEnabled(false);
    attachExtras()->setEnabled(false);
    submitButton()->setText(Strings::feedback_submitting());
    submitButton()->setEnabled(false);
  }

  void showThanks()
  {
    if (m_feedback.joinable())
      m_feedback.join();

    error()->setVisible(false);
    faces()->setVisible(false);
    form()->setVisible(false);
    submitButton()->setVisible(false);
    thankYou()->setVisible(true);
    close()->requestFocus();
    remapWindow();

    m_feedbackInProgress = false;
    m_submitted = true;
  }

  void showError()
  {
    if (m_feedback.joinable())
      m_feedback.join();

    thankYou()->setVisible(false);
    faces()->setVisible(false);
    form()->setVisible(false);
    submitButton()->setText(Strings::feedback_retry());
    submitButton()->setEnabled(true);
    error()->setVisible(true);
    close()->requestFocus();
    remapWindow();

    m_feedbackInProgress = false;
  }

protected:
  void onBeforeClose(CloseEvent& ev) override
  {
    if (m_feedbackInProgress)
      ev.cancel();
  }

private:
  void onSubmit()
  {
    ASSERT(!m_submitted);
    if (m_submitted)
      return;

    showLoading();
    m_feedbackInProgress = true;
    m_feedback = std::thread([this, f = feedback()] {
      base::this_thread::set_name("send-feedback");
      if (Sentry::sendFeedback(f)) {
        execute_from_ui_thread([this] { showThanks(); });
      }
      else {
        execute_from_ui_thread([this] { showError(); });
      }
    });
  };

  void onFaceChange(ButtonSet::Item* = nullptr)
  {
    switch (faces()->selectedItem()) {
      case 0: comments()->setPlaceholder(Strings::feedback_positive_placeholder()); break;
      case 1: comments()->setPlaceholder(Strings::feedback_neutral_placeholder()); break;
      case 2: comments()->setPlaceholder(Strings::feedback_negative_placeholder()); break;
    }

    if (!form()->isVisible()) {
      form()->setVisible(true);
      actionButtons()->setVisible(true);
      remapWindow();
    }

    comments()->requestFocus();
    comments()->selectAll();
    comments()->selectionClear();
  }

  bool m_submitted = false;
  bool m_feedbackInProgress = false;
  std::thread m_feedback;
};
} // namespace
#endif

class FeedbackCommand : public Command {
public:
  FeedbackCommand();

protected:
  void onExecute(Context* context) override;
  bool onEnabled(Context* context) override;

private:
#if ENABLE_SENTRY
  Sentry::Feedback m_lastFeedback;
#endif
};

FeedbackCommand::FeedbackCommand() : Command(CommandId::Feedback())
{
}

bool FeedbackCommand::onEnabled(Context* context)
{
#if ENABLE_SENTRY
  return context->isUIAvailable() && Sentry::isInitialized();
#endif

  return false;
}

void FeedbackCommand::onExecute(Context* context)
{
#ifndef ENABLE_SENTRY
  return;
#else
  FeedbackWindow window(m_lastFeedback);
  window.openWindowInForeground();
  m_lastFeedback = window.feedback();
#endif
}

Command* CommandFactory::createFeedbackCommand()
{
  return new FeedbackCommand;
}

} // namespace app
