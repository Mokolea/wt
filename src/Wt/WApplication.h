// This may look like C code, but it's really -*- C++ -*-
/*
 * Copyright (C) 2008 Emweb bv, Herent, Belgium.
 *
 * See the LICENSE file for terms of use.
 */
#ifndef WAPPLICATION_
#define WAPPLICATION_

#include <chrono>
#include <unordered_map>
#include <vector>
#include <string>
#include <set>

// even boost/poolfwd.hpp includes <windows.h> ...
namespace boost {
  struct default_user_allocator_new_delete;

  template <typename UserAllocator>
  class pool;
}

#include <Wt/WObject.h>
#include <Wt/WCssStyleSheet.h>
#include <Wt/WEnvironment.h>
#include <Wt/WEvent.h>
#include <Wt/WJavaScriptPreamble.h>
#include <Wt/WJavaScriptSlot.h>
#include <Wt/WLocale.h>
#include <Wt/WMessageResourceBundle.h>
#include <Wt/WSignal.h>
#include <Wt/WString.h>

namespace Wt {

#ifndef WT_TARGET_JAVA
/*
 * Symbols used to check that included version matches library version
 * against which you link.
 */
struct WtLibVersion { };
extern WT_API const WtLibVersion WT_INCLUDED_VERSION;
#endif

class WApplication;
class WCombinedLocalizedStrings;
class WContainerWidget;
class WEnvironment;
class WEvent;
class WLoadingIndicator;
class WLogEntry;
class WResource;
class WText;
#ifndef WT_TARGET_JAVA
class WWebSocketResource;
#endif // WT_TARGET_JAVA

class WebSession;
class RootContainer;
class UpdateLockImpl;
class SoundManager;
class ServerSideFontMetrics;

  namespace Http {
    class Cookie;
  }

/*! \brief Typedef for a function that creates WApplication objects.
 *
 * \sa WRun()
 *
 * \relates WApplication
 */
typedef std::function<std::unique_ptr<WApplication> (const WEnvironment&)> ApplicationCreator;

#ifdef WT_TARGET_JAVA
/*! \brief An HTML Meta Header
 */
#endif // WT_TARGET_JAVA
class MetaHeader {
public:
  /*! \brief Constructor
   *
   * Creates a meta header. The lang and user agents are optional, and should
   * be an empty string if not used.
   */
  MetaHeader(MetaHeaderType type, const std::string& name,
             const WString& content, const std::string& lang,
             const std::string& userAgent);

  MetaHeaderType type;
  std::string name, lang, userAgent;
  WString content;
};

/*! \class WApplication Wt/WApplication.h Wt/WApplication.h
 *  \brief Represents an application instance for a single session.
 *
 * \if cpp
 *
 * Each user session of your application has a corresponding
 * %WApplication instance. You need to create a new instance and return
 * it as the result of the callback function passed to WRun(). The
 * instance is the main entry point to session information, and holds
 * a reference to the root() of the widget tree.
 *
 * \elseif java
 *
 * Each user session of your application has a corresponding
 * %WApplication instance. You need to create a new instance and return
 * it as the result of {javadoclink WtServlet#createApplication(WEnvironment)}.
 * The instance is the main entry point to session information,
 * and holds a reference to the root() of the widget tree.
 *
 * \endif
 *
 * The recipe for a %Wt web application, which allocates new
 * WApplication instances for every user visiting the application is
 * thus:
 *
 * \if cpp
 * \code
 * std::unique_ptr<WApplication> createApplication(const WEnvironment& env)
 * {
 *   //
 *   // Optionally, check the environment and redirect to an error page.
 *   //
 *   bool valid = ...;
 *
 *   std::unique_ptr<WApplication> app;
 *   if (!valid) {
 *     app = std::make_unique<WApplication>(env);
 *     app->redirect("error.html");
 *     app->quit();
 *   } else {
 *     // usually you will specialize your application class
 *     app = std::make_unique<WApplication>(env);
 *
 *     //
 *     // Add widgets to app->root() and return the application object.
 *     //
 *   }
 *
 *   return app;
 * }
 * \endcode
 * \elseif java
 * \code
 * public class HelloServlet extends WtServlet {
 *  public HelloServlet() {
 *      super();
 *  }
 *
 *  public WApplication createApplication(WEnvironment env) {
 *      // In practice, you will specialize WApplication and simply
 *      // return a new instance.
 *      WApplication app = new WApplication(env);
 *      app.getRoot().addWidget(new WText("Hello world."));
 *      return app;
 *  }
 * }
 * \endcode
 * \endif
 *
 * \if cpp
 *
 * Throughout the session, the instance is available through
 * WApplication::instance() (or through #wApp). The application may be
 * exited either using the method quit(), or because of a timeout
 * after the user has closed the window, but not because the user does
 * not interact: keep-alive messages in the background will keep the
 * session around as long as the user has the page opened. In either
 * case, the application object is deleted, allowing for cleanup of
 * the entire widget tree, and any other resources.
 *
 * \elseif java
 *
 * Throughout the session, the instance is available through the
 * static method WApplication::instance(), which uses thread-specific
 * storage to keep track of the current session. The application may
 * be exited either using the method quit(), or because of a timeout
 * after the user has closed the window, but not because the user does
 * not interact: keep-alive messages in the background will keep the
 * session around as long as the user has the page opened.
 *
 * \endif
 *
 * The %WApplication object provides access to session-wide settings, including:
 *
 * - circumstantial information through environment(), which gives
 *   details about the user, start-up arguments, and user agent
 *   capabilities.
 * - the application title with setTitle().
 * - inline and external style sheets using styleSheet() and
 *   useStyleSheet().
 * - inline and external JavaScript using doJavaScript() and require().
 * - the top-level widget in root(), representing the entire browser window,
 *   or multiple top-level widgets using bindWidget() when deployed in
 *   EntryPointType::WidgetSet mode to manage a number of widgets within a 3rd party page.
 * - definition of cookies using setCookie() to persist information across
 *   sessions, which may be read using WEnvironment::getCookie() in a future
 *   session.
 * - management of the internal path (that enables browser history and
 *   bookmarks) using setInternalPath() and related methods.
 * - support for server-initiated updates with enableUpdates()
 * \if cpp
 * - localization information and message resources bundles using setLocale()
 *   and messageResourceBundle().
 * \elseif java
 * - localization information and message resources bundles, with
 *   setLocale() and setLocalizedStrings()
 * \endif
 */
class WT_API WApplication : public WObject
{
public:
  /*! \brief Typedef for a function that creates WApplication objects.
   *
   * \sa WRun()
   */
  typedef Wt::ApplicationCreator ApplicationCreator;

  /*! \brief Creates a new application instance.
   *
   * The \p environment provides information on the initial request,
   * user agent, and deployment-related information.
   */
#if defined(DOXYGEN_ONLY) || defined(WT_TARGET_JAVA)
  WApplication(const WEnvironment& environment);
#else
  WApplication(const WEnvironment& environment,
               WtLibVersion version = WT_INCLUDED_VERSION);
#endif

#ifndef WT_TARGET_JAVA
  /*! \brief Destructor.
   *
   * The destructor deletes the root() container, and as a consequence
   * the entire widget tree.
   */
  ~WApplication();
#endif // WT_TARGET_JAVA

  /*! \brief Returns the current application instance.
   *
   * \if cpp
   * This is the same as the global define #wApp. In a multi-threaded server,
   * this method uses thread-specific storage to fetch the current session.
   * \elseif java
   * This method uses thread-specific storage to fetch the current session.
   * \endif
   */
  static WApplication *instance();

  /*! \brief Returns the environment information.
   *
   * This method returns the environment object that was used when
   * constructing the application. The environment provides
   * information on the initial request, user agent, and
   * deployment-related information.
   *
   * \sa url(), sessionId()
   */
  const WEnvironment& environment() const;

  /*! \brief Returns the root container.
   *
   * This is the top-level widget container of the application, and
   * corresponds to entire browser window. The user interface of your
   * application is represented by the content of this container.
   *
   * \if cpp
   *
   * The %root() widget is only defined when the application manages
   * the entire window. When deployed as a \link Wt::EntryPointType::WidgetSet
   * EntryPointType::WidgetSet\endlink application, there is no %root() container, and
   * \c 0 is returned.  Instead, use bindWidget() to bind one or more
   * root widgets to existing HTML &lt;div&gt; (or other) elements on
   * the page.
   *
   * \elseif java
   *
   * The root() widget is only defined when the application manages
   * the entire window. When deployed as a \link Wt::EntryPointType::WidgetSet
   * EntryPointType::WidgetSet\endlink application, there is no %root() container, and
   * <code>null</code> is returned. Instead, use bindWidget() to bind
   * one or more root widgets to existing HTML &lt;div&gt; (or other)
   * elements on the page.
   *
   * \endif
   */
  WContainerWidget *root() const { return widgetRoot_; }

  /*! \brief Finds a widget by name.
   *
   * This finds a widget in the application's widget hierarchy. It
   * does not only consider widgets in the root(), but also widgets
   * that are placed outside this root, such as in dialogs, or other
   * "roots" such as all the bound widgets in a widgetset application.
   *
   * \sa WWidget::setObjectName(), WWidget::find()
   */
  WWidget *findWidget(const std::string& name);

  /** @name Style sheets and CSS
   */
  //!@{
  /*! \brief Returns a reference to the inline style sheet.
   *
   * Widgets may allow configuration of their look and feel through
   * style classes. These may be defined in this inline stylesheet, or
   * in external style sheets.
   *
   * It is usually preferable to use external stylesheets (and
   * consider more accessible). Still, the internal stylesheet has as
   * benefit that style rules may be dynamically updated, and it is
   * easier to manage logistically.
   *
   * \sa useStyleSheet()
   * \sa WWidget::setStyleClass()
   */
  WCssStyleSheet& styleSheet() { return styleSheet_; }

  /*! \brief Adds an external style sheet.
   *
   * The \p link is a link to a stylesheet.
   *
   * The \p media indicates the CSS media to which this stylesheet
   * applies. This may be a comma separated list of media. The default
   * value is "all" indicating all media.
   *
   * This is an overloaded method for convenience, equivalent to:
   * \code
   * useStyleSheet(Wt::WCssStyleSheet(link, media))
   * \endcode
   */
  void useStyleSheet(const WLink& link, const std::string& media = "all");

  /*! \brief Conditionally adds an external style sheet.
   *
   * This is an overloaded method for convenience, equivalent to:
   * \code
   * useStyleSheet(Wt::WLinkedCssStyleSheet(link, media), condition)
   * \endcode
   */
  void useStyleSheet(const WLink& link, const std::string& condition,
                     const std::string& media);

  /*! \brief Adds an external stylesheet.
   *
   * Widgets may allow configuration of their look and feel through
   * style classes. These may be defined in an inline stylesheet,
   * or in external style sheets.
   *
   * External stylesheets are inserted after the internal style sheet,
   * and can therefore override default styles set by widgets in the
   * internal style sheet.
   * External stylesheets must have valid link.
   *
   * If not empty, \p condition is a string that is used to apply the
   * stylesheet to specific versions of IE. Only a limited subset of
   * the IE conditional comments syntax is supported (since these are
   * in fact interpreted server-side instead of client-side). Examples
   * are:
   *
   * - "IE gte 6": only for IE version 6 or later.
   * - "!IE gte 6": only for IE versions prior to IE6.
   * - "IE lte 7": only for IE versions prior to IE7.
   *
   * \sa styleSheet(), useStyleSheet(const std::string&, const std::string&),
   *  removeStyleSheet(const WLink& link)
   * \sa WWidget::setStyleClass()
   */
  void useStyleSheet(const WLinkedCssStyleSheet& styleSheet,
                     const std::string& condition = "");

  /*! \brief Removes an external stylesheet.
   *
   * \sa styleSheet(), useStyleSheet(const std::string&, const std::string&)
   * \sa WWidget::setStyleClass()
   */
  void removeStyleSheet(const WLink& link);

  /*! \brief Sets the theme.
   *
   * The theme provides the look and feel of several built-in widgets,
   * using CSS style rules. Rules for each theme are defined in the
   * <tt>resources/themes/</tt><i>theme</i><tt>/</tt> folder.
   *
   * The default theme is "default" CSS theme.
   */
  void setTheme(const std::shared_ptr<WTheme>& theme);

  /*! \brief Returns the theme.
   */
  std::shared_ptr<WTheme> theme() const { return theme_; }

  /*! \brief Sets a CSS theme.
   *
   * This sets a WCssTheme as theme.
   *
   * The theme provides the look and feel of several built-in widgets,
   * using CSS style rules. Rules for each CSS theme are defined in
   * the <tt>resources/themes/</tt><i>name</i><tt>/</tt> folder.
   *
   * The default theme is "default". Setting an empty theme "" will
   * result in a stub CSS theme that does not load any stylesheets.
   */
  void setCssTheme(const std::string& name);

  /*! \brief Sets the layout direction.
   *
   * The default direction is LayoutDirection::LeftToRight.
   *
   * This sets the language text direction, which by itself sets the
   * default text alignment and reverse the column orders of &lt;table&gt;
   * elements.
   *
   * In addition, %Wt will take this setting into account in
   * WTextEdit, WTableView and WTreeView (so that columns are
   * reverted), and swap the behaviour of WWidget::setFloatSide() and
   * WWidget::setOffsets() for LayoutDirection::RightToLeft
   * languages. Note that CSS settings themselves are not affected by
   * this setting, and thus for example <tt>"float: right"</tt> will
   * move a box to the right, irrespective of the layout direction.
   *
   * The library sets <tt>"Wt-ltr"</tt> or <tt>"Wt-rtl"</tt> as style
   * classes for the document body. You may use this if to override
   * certain style rules for a Right-to-Left document.
   *
   * The only valid values are LayoutDirection::LeftToRight or
   * LayoutDirection::RightToLeft.
   *
   * For example:
   * \code
   * body        .sidebar { float: right; }
   * body.Wt-rtl .sidebar { float: left; }
   * \endcode
   *
   * \note The layout direction can be set only at application startup
   * and does not have the effect of rerendering the entire UI.
   */
  void setLayoutDirection(LayoutDirection direction);

  /*! \brief Returns the layout direction.
   *
   * \sa setLayoutDirection()
   */
  LayoutDirection layoutDirection() const { return layoutDirection_; }

  /*! \brief Sets a style class to the entire page &lt;body&gt;.
   *
   * \sa setHtmlClass()
   */
  void setBodyClass(const std::string& styleClass);

  /*! \brief Returns the style class set for the entire page &lt;body&gt;.
   *
   * \sa setBodyClass()
   */
  std::string bodyClass() const { return bodyClass_; }

  /*! \brief Sets a style class to the entire page &lt;html&gt;.
   *
   * \sa setBodyClass()
   */
  void setHtmlClass(const std::string& styleClass);

  /*! \brief Returns the style class set for the entire page &lt;html&gt;.
   *
   * \sa setHtmlClass()
   */
  std::string htmlClass() const { return htmlClass_; }

  /*! \brief Sets an attribute for the entire page &lt;html&gt; element.
   *
   * This allows you to set any of the global attributes (see:
   * https://developer.mozilla.org/en-US/docs/Web/HTML/Global_attributes)
   * on the &lt;html&gt; tag. As well as any tags specific to that tag
   * (see: https://developer.mozilla.org/en-US/docs/Web/HTML/Element/html).
   *
   * \note If the \p value contains more complex JavaScript, make sure
   * that \p " and \p ' are properly escaped. Otherwise you may encounter
   * JavaScript errors.
   *
   * \note This can control the &lt;html&gt;'s \p class, \p dir, and
   * \p lang as well, but this should generally be avoided, since the
   * application manages that separately.
   *
   * \sa htmlAttribute(), setBodyAttribute()
   */
  void setHtmlAttribute(const std::string& name, const std::string& value);

  /*! \brief Returns the current &lt;html&gt; element attribute value of
   * the specified \p name.
   *
   * \sa setHtmlAttribute(), bodyAttribute()
   */
  WString htmlAttribute(const std::string& name) const;

  /*! \brief Sets an attribute for the entire page &lt;body&gt; element.
   *
   * This allows you to set any of the global attributes (see:
   * https://developer.mozilla.org/en-US/docs/Web/HTML/Global_attributes)
   * on the &lt;body&gt; tag. As well as any tags specific to that tag
   * (see: https://developer.mozilla.org/en-US/docs/Web/HTML/Element/body).
   *
   * \note If the \p value contains more complex JavaScript, make sure
   * that \p " and \p ' are properly escaped. Otherwise you may encounter
   * JavaScript errors.
   *
   * \sa bodyAttribute(), setHtmlAttribute()
   */
  void setBodyAttribute(const std::string& name, const std::string& value);

  /*! \brief Returns the current &lt;body&gt; element attribute value of
   * the specified \p name.
   *
   * \sa setBodyAttribute(), htmlAttribute()
   */
  WString bodyAttribute(const std::string& name) const;
  //!@}

  /*! \brief Sets the window title.
   *
   * Sets the browser window title to \p title.
   *
   * The default title is "".
   *
   * \sa title()
   */
  void setTitle(const WString& title);

  /*! \brief Returns the window title.
   *
   * \sa setTitle(const WString&)
   */
  const WString& title() const { return title_; }

  /*! \brief Returns the close message.
   *
   * \sa setConfirmCloseMessage()
   */
  const WString& closeMessage() const { return closeMessage_; }

  /*! \brief Returns the resource object that provides localized strings.
   *
   * \if cpp
   * The default value is a WMessageResourceBundle instance, which
   * uses XML files to resolve localized strings, but you can set a
   * custom class using setLocalizedStrings().
   * \elseif java
   * This returns the object previously set using setLocalizedStrings().
   * \endif
   *
   * WString::tr() is used to create localized strings, whose
   * localized translation is looked up through this object, using a
   * key.
   *
   * \if cpp
   * \sa WString::tr(), messageResourceBundle()
   * \elseif java
   * \sa WString::tr()
   * \endif
   */
  std::shared_ptr<WLocalizedStrings> localizedStrings();

#ifdef WT_TARGET_JAVA
  /*! \brief Accesses the built-in resource bundle.
   *
   * This is an internal function and should not be called directly.
   *
   * \sa localizedStrings()
   */
#endif // WT_TARGET_JAVA
  WMessageResourceBundle& builtinLocalizedStrings();

  /*! \brief Sets the resource object that provides localized strings.
   *
   * The \p translator resolves localized strings within the current
   * application locale.
   *
   * \sa localizedStrings(), WString::tr(const char *key)
   */
  void setLocalizedStrings(const std::shared_ptr<WLocalizedStrings>&
                           stringResolver);

#ifndef WT_TARGET_JAVA
  /*! \brief Returns the message resource bundle.
   *
   * The message resource bundle defines the list of external XML
   * files that are used to lookup localized strings.
   *
   * The default localizedStrings() is a WMessageResourceBundle
   * object, and this method returns localizedStrings() downcasted to
   * this type.
   *
   * \throws WException when localizedStrings() is not a WMessageResourceBundle
   *
   * \sa WString::tr(const char *key)
   */
  WMessageResourceBundle& messageResourceBundle();
#endif // WT_TARGET_JAVA

  /*! \brief Changes the locale.
   *
   * The locale is used by the localized strings resource to resolve
   * localized strings.
   *
   * By passing an empty \p locale, the default locale is
   * chosen.
   *
   * By default, when the locale is changed, refresh() is called, 
   * which will resolve the strings of the current user-interface
   * in the new locale. This can be changed by having the 
   * \p doRefresh parameter set to \p false.
   *
   * At construction, the locale is copied from the environment
   * (WEnvironment::locale()), and this is the locale that was
   * configured by the user in his browser preferences, and passed
   * using an HTTP request header.
   *
   * \sa localizedStrings(), WString::tr()
   */
  void setLocale(const WLocale& locale, bool doRefresh = true);

  /*! \brief Returns the current locale.
   *
   * \sa setLocale(const WLocale&)
   */
  const WLocale& locale() const { return locale_; }

  /*! \brief Refreshes the application.
   *
   * This lets the application refresh its data, including strings
   * from message resource bundles. This is done by propagating
   * WWidget::refresh() through the widget hierarchy.
   *
   * This method is also called when the user hits the refresh (or
   * reload) button, if this can be caught within the current session.
   *
   * \if cpp
   *
   * The reload button may only be caught when %Wt is configured so that
   * reload should not spawn a new session. When URL rewriting is used for
   * session tracking, this will cause an ugly session ID to be added to the
   * URL. See \ref config_session for configuring the reload
   * behavior ("<reload-is-new-session>").
   *
   * \elseif java
   *
   * The reload button may only be caught when cookies for session
   * tracking are configured in the servlet container.
   *
   * \endif
   *
   * \sa WWidget::refresh()
   */
  virtual void refresh();

  /*! \brief Binds a top-level widget for a EntryPointType::WidgetSet deployment.
   *
   * This method binds a \p widget to an existing element with DOM id
   * \p domId on the page. The element type should correspond with
   * the widget type (e.g. it should be a &lt;div&gt; for a
   * WContainerWidget, or a &lt;table&gt; for a WTable).
   *
   * \sa root()
   * \sa Wt::EntryPointType::WidgetSet
   */
  void bindWidget(std::unique_ptr<WWidget> widget, const std::string& domId);

  /** @name URLs and internal paths
   */
  //!@{
  /*! \brief Returns a URL for the current session
   *
   * Returns the (relative) URL for this application session
   * (including the session ID if necessary). The URL includes the
   * full application path, and is expanded by the browser into a full
   * URL.
   *
   * For example, for an application deployed at \code
   * http://www.mydomain.com/stuff/app.wt \endcode this method might
   * return <tt>"/stuff/app.wt?wtd=AbCdEf"</tt>. Additional query
   * parameters can be appended in the form of
   * <tt>"&param1=value&param2=value"</tt>.
   *
   * To obtain a URL that is suitable for bookmarking the current
   * application state, to be used across sessions, use bookmarkUrl()
   * instead.
   *
   * \sa redirect(), WEnvironment::hostName(), WEnvironment::urlScheme()
   * \sa bookmarkUrl()
   */
  std::string url(const std::string& internalPath = std::string()) const;

  /*! \brief Makes an absolute URL.
   *
   * Returns an absolute URL for a given (relative url) by including
   * the schema, hostname, and deployment path.
   *
   * If \p url is "", then the absolute base URL is returned. This is
   * the absolute URL at which the application is deployed, up to the
   * last '/'.
   *
   * The default implementation is not complete: it does not handle relative
   * URL path segments with '..'. It just handles the cases that are necessary for
   * %Wt.
   *
   * This is not used in the library, except when a public URL is
   * needed, e.g. for inclusion in an email.
   *
   * You may want to reimplement this method when the application is
   * hosted behind a reverse proxy or in general the public URL of the
   * application cannot be guessed correctly by the application.
   */
  virtual std::string makeAbsoluteUrl(const std::string& url) const;

  /*! \brief "Resolves" a relative URL taking into account internal paths.
   *
   * This resolves the relative URL against the base path of the application,
   * so that it will point to the correct path regardless of the current internal
   * path, e.g. if the application is deployed at <tt>%http://example.com/one</tt>
   * and we're at the internal path <tt>/two/</tt>, so that the full URL is
   * <tt>%http://example.com/one/two/</tt>, the output of the input URL <tt>three</tt>
   * will point to <tt>%http://example.com/three</tt>, and not
   * <tt>%http://example.com/one/two/three</tt>.
   *
   * If the given url is the empty string, the result will point to the base path
   * of the application.
   *
   * See the table below for more examples.
   *
   * <h3>When you would want to use resolveRelativeUrl:</h3>
   *
   * Using HTML5 History API or in a plain HTML session (without ugly
   * internal paths), the internal path is present as a full part of
   * the URL. This has a consequence that relative URLs, if not dealt
   * with, would be resolved against the last 'folder' name of the
   * internal path, rather than against the application deployment
   * path (which is what you probably want).
   *
   * When using a widgetset mode deployment, or when configuring a
   * baseURL property in the configuration, this method will make an
   * absolute URL so that the property is fetched from the right
   * server.
   *
   * Otherwise, this method will fixup a relative URL so that it
   * resolves correctly against the base path of an application. This
   * does not necessarily mean that the URL is resolved into an
   * absolute URL. In fact, %Wt will simply prepend a sequence of "../"
   * path elements to correct for the internal path. When passed an
   * absolute URL (i.e. starting with '/'), the url is returned
   * unchanged.
   *
   * For URLs passed to the %Wt API (and of which the library knows it
   * represents a URL) this method is called internally by the
   * library. But it may be useful for URLs which are set e.g. inside
   * a WTemplate.
   *
   * <h3>Examples</h3>
   *
   * Note that whether the deployment path and entry point ends with a slash is significant.
   * Below are some examples with the
   * <span style="color:red;">deployment path</span> in <span style="color:red;">red</span>
   * and the <span style="color:blue;">internal path</span> in <span style="color:blue;">blue</span>.
   *
   * <table>
   * <tr><th>Current full path</th><th>url argument</th><th>Result points to</th></tr>
   * <tr><td rowspan="4"><tt>%http://example.com</tt><tt style="color:red;">/foo/bar</tt><tt style="color:blue;">/internal/path</tt><br />Deployment path: <tt style="color:red;">/foo/bar</tt> (no slash at the end)<br />Internal path: <tt style="color:blue;">/internal/path</tt></td>
   * <td><i>(empty string)</i></td><td><tt>%http://example.com/foo/bar</tt></tr>
   * <tr><td><tt>.</tt></td><td><tt>%http://example.com/foo/</tt></td></tr>
   * <tr><td><tt>./</tt></td><td><tt>%http://example.com/foo/</tt></td></tr>
   * <tr><td><tt>../</tt></td><td><tt>%http://example.com/</tt></td></tr>
   * <tr><td rowspan="4"><tt>%http://example.com</tt><tt style="color:red;">/foo/bar</tt><tt style="color:magenta;font-weight:bold;">/</tt><tt style="color:blue;">internal/path</tt><br />Deployment path: <tt style="color:red;">/foo/bar/</tt> (with slash at the end)<br />Internal path: <tt style="color:blue;">/internal/path</tt><br />Note that the slash between the deployment path and the internal path is shared</td>
   * <td><i>(empty string)</i></td><td><tt>%http://example.com/foo/bar/</tt></tr>
   * <tr><td><tt>.</tt></td><td><tt>%http://example.com/foo/bar/</tt></td></tr>
   * <tr><td><tt>./</tt></td><td><tt>%http://example.com/foo/bar/</tt></td></tr>
   * <tr><td><tt>../</tt></td><td><tt>%http://example.com/foo/</tt></td></tr>
   * </table>
   */
  std::string resolveRelativeUrl(const std::string& url) const;

  /*! \brief Returns a bookmarkable URL for the current internal path.
   *
   * Is equivalent to <tt>bookmarkUrl(internalPath())</tt>, see
   * bookmarkUrl(const std::string&) const.
   *
   * To obtain a URL that is refers to the current session of the
   * application, use url() instead.
   *
   * \sa url(), bookmarkUrl(const std::string&) const
   */
  std::string bookmarkUrl() const;

  /*! \brief Returns a bookmarkable URL for a given internal path.
   *
   * Returns the (relative) URL for this application that includes the
   * internal path \p internalPath, usable across sessions.
   *
   * The returned URL concatenates the internal path to the application
   * base URL, and when no JavaScript is available and URL rewriting is used
   * for session-tracking, a session Id is appended to reuse an existing
   * session if available.
   *
   * \if cpp
   * See also \ref config_session for configuring the session-tracking
   * method.
   *
   * For the built-in httpd, when the application is deployed at a folder
   * (ending with '/'), only an exact matching path is routed to
   * the application (this can be changed since Wt 3.1.9, see
   * \ref wthttpd ), making clean URLs impossible. Returned URLs then
   * include a <tt>"?_="</tt> encoding for the internal path.
   * \endif
   *
   * You can use bookmarkUrl() as the destination for a WAnchor, and
   * listen to a click event is attached to a slot that switches to
   * the internal path \p internalPath (see
   * WAnchor::setRefInternalPath()). In this way, an anchor can be
   * used to switch between internal paths within an application
   * regardless of the situation (browser with or without Ajax
   * support, or a web spider bot), but still generates suitable URLs
   * across sessions, which can be used for bookmarking, opening in a
   * new window/tab, or indexing.
   *
   * To obtain a URL that refers to the current session of the
   * application, use url() instead.
   *
   * \sa url(), bookmarkUrl()
   *
   * \if cpp
   * \note the \p internalPath should be CharEncoding::UTF8 encoded (we may fix the API
   *       to use WString in the future).
   * \endif
   */
  std::string bookmarkUrl(const std::string& internalPath) const;

  /*! \brief Changes the internal path.
   *
   * A %Wt application may manage multiple virtual paths. The virtual
   * path is appended to the application URL. Depending on the
   * situation, the path is directly appended to the application URL
   * or it is appended using a name anchor (#).
   *
   * For example, for an application deployed at:
   * \code
   * http://www.mydomain.com/stuff/app.wt
   * \endcode
   * for which an \p internalPath <tt>"/project/z3cbc/details/"</tt> is
   * set, the two forms for the application URL are:
   * <ul>
   * <li> in an AJAX session (HTML5):
   * \code
   * http://www.mydomain.com/stuff/app.wt/project/z3cbc/details/
   * \endcode
   * </li><li> in an AJAX session (HTML4):
   * \code
   * http://www.mydomain.com/stuff/app.wt#/project/z3cbc/details/
   * \endcode
   * </li><li>
   * in a plain HTML session:
   * \code
   * http://www.mydomain.com/stuff/app.wt/project/z3cbc/details/
   * \endcode
   * </li></ul>
   *
   * Note, since %Wt 3.1.9, the actual form of the URL no longer
   * affects relative URL resolution, since now %Wt includes an HTML
   * <tt>meta base</tt> tag which points to the deployment path,
   * regardless of the current internal path. This does break
   * deployments behind a reverse proxy which changes paths.
   *
   * \if cpp
   * For the built-in httpd, when the application is deployed
   * at a folder (ending with '/'), only an exact matching path is
   * routed to the application (this can be changed since Wt 3.1.9,
   * see \ref wthttpd ), making clean URLs impossible. Returned
   * URLs then include a <tt>"?_="</tt> encoding for the internal
   * path:
   *
   * \code
   * http://www.mydomain.com/stuff/?_=/project/z3cbc/details/
   * \endcode
   * \endif
   *
   * When the internal path is changed, an entry is added to the
   * browser history. When the user navigates back and forward through
   * this history (using the browser back/forward buttons), an
   * internalPathChanged() event is emitted. You should listen to this
   * signal to switch the application to the corresponding state. When
   * \p emitChange is \c true, this signal is also emitted by setting
   * the path (but only if the path is actually changed).
   *
   * A url that includes the internal path may be obtained using
   * bookmarkUrl().
   *
   * The \p internalPath must start with a '/'. In this way, you
   * can still use normal anchors in your HTML. Internal path changes
   * initiated in the browser to paths that do not start with a '/'
   * are ignored.
   *
   * The \p emitChange parameter determines whether calling this
   * method causes the internalPathChanged() signal to be emitted.
   *
   * \sa bookmarkUrl(), internalPath(), internalPathChanged()
   *
   * \if cpp
   * \note the \p path should be CharEncoding::UTF8 encoded (we may fix the API
   *       to use WString in the future).
   * \endif
   */
  void setInternalPath(const std::string& path, bool emitChange = false);

  /*! \brief Sets whether an internal path is valid by default.
   *
   * This configures how you treat (invalid) internal paths. If an
   * internal path is treated valid by default then you need to call
   * setInternalPath(false) for an invalid path. If on the other hand
   * you treat an internal path as invalid by default, then you need
   * to call setInternalPath(true) for a valid path.
   *
   * A user which opens an invalid internal path will receive a HTTP
   * 404-Not Found response code (if sent an HTML response).
   *
   * The default value is \c true.
   */
  void setInternalPathDefaultValid(bool valid);

  /*! \brief Returns whether an internal path is valid by default.
   *
   * \sa setInternalPathDefaultValid()
   */
  bool internalPathDefaultValid() const { return internalPathDefaultValid_; }

  /*! \brief Sets whether the current internal path is valid.
   *
   * You can use this function in response to an internal path change
   * event (or at application startup) to indicate whether the new (or
   * initial) internal path is valid. This has only an effect on plain
   * HTML sessions, or on the first response in an application
   * deployed with progressive bootstrap settings, as this generates
   * then a 404 Not-Found response.
   *
   * \sa internalPathChanged(), setInternalPathDefaultValid()
   */
  void setInternalPathValid(bool valid);

  /*! \brief Returns whether the current internal path is valid.
   *
   * \sa setInternalPathValid()
   */
  bool internalPathValid() const { return internalPathValid_; }

  /*! \brief Returns the current internal path.
   *
   * When the application is just created, this is equal to
   * WEnvironment::internalPath().
   *
   * \sa setInternalPath(), internalPathNextPart(), internalPathMatches()
   *
   * \if cpp
   * \note the \p returned path is CharEncoding::UTF8 (we may fix the API
   *       to use WString in the future).
   * \endif
   */
  std::string internalPath() const;

  /*! \brief Returns a part of the current internal path.
   *
   * This is a convenience method which returns the next \p folder
   * in the internal path, after the given \p path.
   *
   * For example, when the current internal path is
   * <tt>"/project/z3cbc/details"</tt>, this method returns
   * <tt>"details"</tt> when called with <tt>"/project/z3cbc/"</tt> as
   * \p path argument.
   *
   * The \p path must start with a '/', and internalPathMatches()
   * should evaluate to \c true for the given \p path. If not,
   * an empty string is returned and an error message is logged.
   *
   * \sa internalPath(), internalPathChanged()
   *
   * \if cpp
   * \note the \p internal path is CharEncoding::UTF8 encoded (we may fix the API
   *       to use WString in the future).
   * \endif
   */
  std::string internalPathNextPart(const std::string& path) const;

  std::string internalSubPath(const std::string& path) const;

  /*! \brief Checks if the internal path matches a given path.
   *
   * Returns whether the current internalPath() starts with
   * \p path (or is equal to \p path). You will typically use
   * this method within a slot conneted to the internalPathChanged()
   * signal, to check that an internal path change affects the
   * widget. It may also be useful before changing \p path using
   * setInternalPath() if you do not intend to remove sub paths when
   * the current internal path already matches \p path.
   *
   * The \p path must start with a '/'.
   *
   * \sa setInternalPath(), internalPath()
   *
   * \if cpp
   * \note the \p internal path is CharEncoding::UTF8 encoded (we may fix the API
   *       to use WString in the future).
   * \endif
   */
  bool internalPathMatches(const std::string& path) const;

  /*! \brief %Signal which indicates that the user changes the internal path.
   *
   * This signal indicates a change to the internal path, which is
   * usually triggered by the user using the browser back/forward
   * buttons.
   *
   * The argument contains the new internal path.
   *
   * \sa setInternalPath()
   *
   * \if cpp
   * \note the \p internal path is CharEncoding::UTF8 encoded (we may fix the API
   *       to use WString in the future).
   * \endif
   */
  Signal<std::string>& internalPathChanged();

  /*! \brief %Signal which indicates that an invalid internal path is navigated.
   */
  Signal<std::string>& internalPathInvalid() { return internalPathInvalid_; }

  /*! \brief Redirects the application to another location.
   *
   * The client will be redirected to a new location identified by \p
   * url. Use this in conjunction with quit() if you want the
   * application to be terminated as well.
   *
   * Calling %redirect() does not imply %quit() since it may be useful
   * to switch between a non-secure and secure (SSL) transport
   * connection.
   */
  void redirect(const std::string& url);
  //!@}

  /*! \brief Returns the URL at which the resources are deployed.
   *
   * Returns resolveRelativeUrl(relativeResourcesUrl())
   */
  static std::string resourcesUrl();

  /*! \brief Returns the URL at which the resources are deployed.
   *
   * \if cpp
   * This returns the value of the 'resources' property set in the
   * configuration file, and may thus be a URL relative to the deployment
   * path.
   * \endif
   *
   * \sa resolveRelativeUrl()
   */
  static std::string relativeResourcesUrl();

#ifndef WT_TARGET_JAVA
  /*! \brief Returns the appRoot special property
   *
   * This returns the "appRoot" property, with a trailing slash added
   * to the end if it was not yet present.
   *
   * The property "appRoot" was introduced as a generalization of the
   * working directory for the location of files that do not need to
   * be served over http to the client, but are required by the
   * program to run properly. Typically, these are message resource
   * bundles (xml), CSV files, database files (e.g. SQLite files for
   * Wt::Dbo), ...
   *
   * Some connectors do not allow you to control what the current
   * working directory (CWD) is set to (fcgi, isapi). Instead of
   * referring to files assuming a sensible CWD, it is therefore
   * better to refer to them relative to the application root.
   *
   * The appRoot property is special in the sense that it can be set
   * implicitly by the connector (see the connector documentation for
   * more info). If it was not set by the connector, it can be set as
   * a normal property in the configuration file (the default
   * wt_config.xml describes how to set properties). If the property
   * is not set at all, it is assumed that the appRoot is CWD and this
   * function will return an empty string.
   *
   * \if cpp
   * Usage example:
   * \code
   * messageResourceBundle().use(appRoot() + "text");
   * messageResourceBundle().use(appRoot() + "charts");
   *
   * auto sqlite3_ = std::make_unique<Wt::Dbo::backend::Sqlite3>(appRoot() + "planner.db");
   * \endcode
   * \endif
   *
   * \sa WServer::appRoot(), docRoot()
   */
  static std::string appRoot();

  /*! \brief Returns the server document root.
   *
   * This returns the filesystem path that corresponds to the document root
   * of the webserver.
   *
   * \note This does not work reliably for complex webserver configurations
   *       (e.g. using FastCGI with Apache and rewrite rules). See also the
   *       <a href="https://issues.apache.org/bugzilla/show_bug.cgi?id=26052">
   *       discussion here</a>.
   *
   * \sa appRoot()
   */
  std::string docRoot() const;

  /*! \brief Sets a client-side connection monitor
   *
   * This can be used to be notified, in the browser, of changes in
   * connection state between the browser and the server. The passed
   * \p jsObject should be an object that has the following prototype:
   * \code
   *  {
   *     onChange: function(type, oldValue, newValue) { ... }
   *  }
   * \endcode
   *
   * The 'onChange' function will be called on an connection status change
   * event. The following types are defined:
   *  - "connectionStatus": 0 = disconnected, 1 = connected
   *  - "websocket": true = websocket is used, false = websocket is not used
   *
   * The current state is also stored in a 'status' object inside the
   * connection monitor.
   *
   * Example usage:
   * \code
   * app.setConnectionMonitor("{ onChange: function(type, ov, nv) { console.log(type, ov, nv); } }");
   * \endcode
   */
  void setConnectionMonitor(const std::string& jsObject);

#else
  static std::string appRoot();
#endif // WT_TARGET_JAVA

  /*! \brief Returns the unique identifier for the current session.
   *
   * The session id is a string that uniquely identifies the current session.
   * Note that the actual contents has no particular meaning and client
   * applications should in no way try to interpret its value.
   */
  std::string sessionId() const;

#ifndef WT_TARGET_JAVA
  /*! \brief Changes the session id.
   *
   * To mitigate session ID fixation attacks, you should use this
   * method to change the session ID to a new random value after a
   * user has authenticated himself.
   *
   * \sa sessionId()
   */
  void changeSessionId();
#endif // WT_TARGET_JAVA

  WebSession *session() const { return session_; }

  /** @name Manipulation outside the main event loop
   */
  //!@{
  /*! \brief Enables server-initiated updates.
   *
   * By default, updates to the user interface are possible only at
   * startup, during any event (in a slot), or at regular time points
   * using WTimer. This is the normal %Wt event loop.
   *
   * In some cases, one may want to modify the user interface from a
   * second thread, outside the event loop. While this may be worked
   * around by the WTimer, in some cases, there are bandwidth and
   * processing overheads associated which may be unnecessary, and
   * which create a trade-off with time resolution of the updates.
   *
   * When \p enabled is \c true, this enables "server push" (what is
   * called 'comet' in AJAX terminology). Widgets may then be
   * modified, created or deleted outside of the event loop (e.g. in
   * response to execution of another thread), and these changes are
   * propagated by calling triggerUpdate().
   *
   * \if cpp
   * There are two ways for safely manipulating a session's UI, with
   * respect to thread-safety and application life-time (the library
   * can decide to terminate an application if it lost connectivity
   * with the browser).
   *
   * <h3>WServer::post()</h3>
   *
   * The easiest and less error-prone solution is to post an event,
   * represented by a function/method call, to a session using
   * WServer::post().
   *
   * The method is non-blocking: it returns immediately, avoiding
   * dead-lock scenarios. The function is called from within a thread
   * of the server's thread pool, and not if the session has been or
   * is being terminated. The function is called in the context of the
   * targeted application session, and with exclusive access to the
   * session.
   *
   * <h3>WApplication::UpdateLock</h3>
   *
   * A more direct approach is to grab the application's update lock and
   * manipulate the application's state directly from another thread.
   *
   * At any time, the application may be deleted (e.g. because of a
   * time out or because the user closes the application window). You
   * should thus make sure you do no longer reference an application
   * after it has been deleted. When %Wt decides to delete an
   * application, it first runs WApplication::finalize() and then
   * invokes the destructor. While doing this, any other thread trying
   * to grab the update lock will unblock, but the lock will return \c
   * false. You should therefore always check whether the lock is
   * valid.
   *
   * \elseif java
   *
   * Note that you need to grab the application's update lock to avoid
   * concurrency problems, whenever you modify the application's state
   * from another thread.
   *
   * \endif
   *
   * An example of how to modify the widget tree outside the event loop
   * and propagate changes is:
   * \if cpp
   * \code
   * // You need to have a reference to the application whose state
   * // you are about to manipulate.
   * // You should prevent the application from being deleted somehow,
   * // before you could grab the application lock.
   * Wt::WApplication *app = ...;
   *
   * {
   *   // Grab the application lock. It is a scoped lock.
   *   Wt::WApplication::UpdateLock lock(app);
   *
   *   if (lock) {
   *     // We now have exclusive access to the application: we can safely modify the widget tree for example.
   *     app->root()->addWidget(std::make_unique<Wt::WText>("Something happened!"));
   *
   *     // Push the changes to the browser
   *     app->triggerUpdate();
   *   }
   * }
   * \endcode
   * \elseif java
   * \code
   * // You need to have a reference to the application whose state
   * // you are about to manipulate.
   * WApplication app = ...;
   *
   * // Grab the application lock
   * WApplication.UpdateLock lock = app.getUpdateLock();
   *
   * try {
   *   // We now have exclusive access to the application:
   *   // we can safely modify the widget tree for example.
   *   app.getRoot().addWidget(new WText("Something happened!"));
   *
   *   // Push the changes to the browser
   *   app.triggerUpdate();
   * } finally {
   *   lock.release();
   * }
   * \endcode
   * \endif
   *
   * \if java
   * This works only if your servlet container supports the Servlet 3.0
   * API. If you try to invoke this function on a servlet container with
   * no such support, an exception will be thrown.
   * \endif
   *
   * \note This works only if JavaScript is available on the client.
   *
   * \sa triggerUpdate()
   */
  void enableUpdates(bool enabled = true);

  /*! \brief Returns whether server-initiated updates are enabled.
   *
   * \sa enableUpdates()
   */
  bool updatesEnabled() const { return serverPush_ > 0; }

  /*! \brief Propagates server-initiated updates.
   *
   * When the lock to the application is released, changes will
   * propagate to the user interface. This call only has an effect
   * after updates have been enabled from within the normal event loop
   * using enableUpdates().
   *
   * This is typically used only outside of the main event loop,
   * e.g. from another thread or from within a method posted to an
   * application using WServer::post(), since changes always propagate
   * within the event loop at the end of the event.
   *
   * The update is not immediate, and thus changes that happen after this
   * call will equally be pushed to the client.
   *
   * \sa enableUpdates()
   */
  void triggerUpdate();

#ifndef WT_TARGET_JAVA
  /*! \brief A RAII lock for manipulating and updating the
   *         application and its widgets outside of the event loop.
   *
   * You can use this lock to manipulate widgets outside of the event
   * loop. Inside the event loop (including events posted using
   * WServer::post()), this lock is already held by the library itself.
   *
   * The lock is recursive, so trying to take a lock, while already
   * holding a lock, will not block.
   */
#else
  /*! \brief A synchronization lock for manipulating and updating the
   *         application and its widgets outside of the event loop.
   *
   * You need to take this lock only when you want to manipulate
   * widgets outside of the event loop. LabelOption::Inside the event loop, this
   * lock is already held by the library itself.
   *
   * \sa getUpdateLock()
   */
#endif // WT_TARGET_JAVA
  class WT_API UpdateLock
#ifdef WT_TARGET_JAVA
    : public AutoCloseable
#endif // WT_TARGET_JAVA
  {
  public:
#ifndef WT_TARGET_JAVA
    /*! \brief Creates and locks the given application.
     *
     * The lock guarantees exclusive access to modify the
     * application's state.
     *
     * You should also consider WServer::post() for lock-free
     * communication between different application sessions.
     *
     * As soon as the library decides to destroy the application, the
     * lock will no longer succeed in taking the application lock. You
     * can need to detect this by checking that after the lock is taken,
     * the lock is taken:
     * \code
     * WApplication::UpdateLock lock(app);
     * if (lock) {
     *   // exclusive access to app state
     * }
     * \endcode
     */
    UpdateLock(WApplication *app);

    /*! \brief Tests whether the update lock was succesfully taken.
     *
     * This may return \c false when the library has already decided
     * to destroy the session (but before your application
     * finalizer/destructor has run to notify helper threads that the
     * application is destroyed).
     */
    explicit operator bool() const { return ok_; }

    /*! \brief Releases the lock.
     */
    ~UpdateLock();

#else
    /*! \brief Releases the lock.
     */
    void release();
#endif

#ifdef WT_TARGET_JAVA
    /*! \brief Releases the lock.
     *
     * Calls release()
     *
     * Implemented in order to support the AutoCloseable interface.
     */
    virtual void close();
#endif // WT_TARGET_JAVA

  private:
#ifdef WT_TARGET_JAVA
    UpdateLock(WApplication *app);
    bool createdHandler_;
#endif // WT_TARGET_JAVA

#ifndef WT_TARGET_JAVA
    mutable std::unique_ptr<UpdateLockImpl> impl_;
    bool ok_;
#endif // !WT_TARGET_JAVA

    friend class WApplication;
  };

#ifdef WT_TARGET_JAVA
  /*! \brief Grabs and returns the lock for manipulating widgets outside
   *         the event loop.
   *
   * You need to keep this lock in scope while manipulating widgets
   * outside of the event loop. In normal cases, inside the %Wt event
   * loop, you do not need to care about it.
   *
   * \sa enableUpdates(), triggerUpdate()
   */
  UpdateLock getUpdateLock();
#endif // WT_TARGET_JAVA

  /*! \brief Attach an auxiliary thread to this application.
   *
   * In a multi-threaded environment, WApplication::instance() uses
   * thread-local data to retrieve the application object that
   * corresponds to the session currently being handled by the
   * thread. This is set automatically by the library whenever an
   * event is delivered to the application, or when you use the
   * UpdateLock to modify the application from an auxiliary thread
   * outside the normal event loop.
   *
   * When you want to manipulate the widget tree inside the main event
   * loop, but from within an auxiliary thread, then you cannot use
   * the UpdateLock since this will create an immediate dead
   * lock. Instead, you may attach the auxiliary thread to the
   * application, by calling this method from the auxiliary thread,
   * and in this way you can modify the application from within that
   * thread without needing the update lock.
   *
   * Calling attachThread() with \p attach = \c false, detaches the
   * current thread.
   */
  void attachThread(bool attach = true);
  //!@}

  /** @name Invoking JavaScript or including scripts
   */
  //!@{
  /*! \brief Executes some JavaScript code.
   *
   * This method may be used to call some custom \p javaScript code as
   * part of an event response.
   *
   * This function does not wait until the JavaScript is run, but
   * returns immediately. The JavaScript will be run after the normal
   * event handling, unless \p afterLoaded is set to \c false.
   *
   * In most situations, it's more robust to use
   * WWidget::doJavaScript() however.
   *
   * \sa WWidget::doJavaScript(), declareJavaScriptFunction()
   */
  void doJavaScript(const std::string& javascript, bool afterLoaded = true);

  /*! \brief Adds JavaScript statements that should be run continuously.
   *
   * This is an internal method.
   *
   * It is used by for example layout managers to adjust the layout
   * whenever the DOM tree is manipulated.
   *
   * \sa doJavaScript()
   */
  void addAutoJavaScript(const std::string& javascript);

  /*! \brief Declares an application-wide JavaScript function.
   *
   * The function is stored in WApplication::javaScriptClass().
   *
   * The next code snippet declares and invokes function foo:
   * \if cpp
   * \code
   * app->declareJavaScriptFunction("foo",
   *                                "function(id) { ... }");
   * ...
   * std::string id("myId");
   * app->doJavaScript(app->javaScriptClass() + ".foo('" + id + "');");
   * \endcode
   * \endif
   */
  void declareJavaScriptFunction(const std::string& name,
                                 const std::string& function);

  /*! \brief Loads a JavaScript library.
   *
   * Loads a JavaScript library located at the URL \p url. %Wt keeps
   * track of libraries (with the same URL) that already have been
   * loaded, and will load a library only once. In addition, you may
   * provide a \p symbol which if already defined will also indicate
   * that the library was already loaded (possibly outside of %Wt when
   * in EntryPointType::WidgetSet mode).
   *
   * This method returns \c true only when the library is loaded
   * for the first time.
   *
   * JavaScript libraries may be loaded at any point in time. Any
   * JavaScript code is deferred until the library is loaded, except
   * for JavaScript that was defined to load before, passing \c false
   * as second parameter to doJavaScript().
   */
  bool require(const std::string& url,
               const std::string& symbol = std::string());

  /*! \brief Loads a custom JQuery library.
   *
   * For %Wt versions before 4.9.0, this function was used to load a different version of jQuery than
   * the one included in %Wt. Since %Wt 4.9.0 however, %Wt no longer relies on jQuery and does not load
   * jQuery by default. If your application relies on jQuery, use require() instead.
   *
   * Calling this function makes customJQuery() return `true`, and calls:
   *
   * ```cpp
   * return require(url, "$");
   * ```
   *
   * \deprecated %Wt no longer loads jQuery by default, making a separate
   *             requireJQuery() function unnecessary, use require() instead
   */
  WT_DEPRECATED("Wt no longer loads jQuery by default, rendering requireJQuery() obsolete, use require() instead")
  bool requireJQuery(const std::string& url);

  /*! \brief Returns whether a custom JQuery library is used.
   *
   * \sa requireJQuery(const std::string& url)
   *
   * \deprecated %Wt no longer loads jQuery by default, making the use of requireJQuery() and thus
   *             customJQuery() obsolete.
   */
  WT_DEPRECATED("Wt no longer loads jQuery by default, rendering requireJQuery() and thus customJQuery() obsolete")
  bool customJQuery() const { return customJQuery_; }

  /*! \brief Sets the name of the application JavaScript class.
   *
   * This should be called right after construction of the application, and
   * changing the JavaScript class is only supported for EntryPointType::WidgetSet mode
   * applications. The \p className should be a valid JavaScript identifier, and
   * should also be unique in a single page.
   */
  void setJavaScriptClass(const std::string& className);

  /*! \brief Returns the name of the application JavaScript class.
   *
   * This JavaScript class encapsulates all JavaScript methods
   * specific to this application instance. The method is foreseen to
   * allow multiple applications to run simultaneously on the same
   * page in Wt::WidgtSet mode, without interfering.
   */
  std::string javaScriptClass() { return javaScriptClass_; }
  //!@}

  /*! \brief Processes UI events.
   *
   * You may call this method during a long operation to:
   *   - propagate widget changes to the client.
   *   - process UI events.
   *
   * This method starts a recursive event loop, blocking the current
   * thread, and resumes when all pending user interface events have been
   * processed.
   *
   * Because a thread is blocked, this may affect your application
   * scalability.
   */
  void processEvents();

  /*! \brief Blocks the thread, waiting for an UI event.
   *
   * This function is used by functions like WDialog::exec() or
   * WPopupMenu::exec(), to block the current thread waiting for a new
   * event.
   *
   * This requires that at least one additional thread is available to
   * process incoming requests, and is not scalable when working with
   * a fixed size thread pools.
   */
  virtual void waitForEvent();

#ifndef WT_TARGET_JAVA
  /*! \brief Reads a configuration property.
   *
   * Tries to read a configured value for the property
   * \p name. The method returns whether a value is defined for
   * the property, and sets it to \p value.
   *
   * \sa WServer::readConfigurationProperty()
   */
  static bool readConfigurationProperty(const std::string& name,
                                        std::string& value);
#else
  /*! \brief Reads a configuration property.
   *
   * Tries to read a configured value for the property
   * \p name. If no value was configured, the default \p value
   * is returned.
   */
  static std::string *readConfigurationProperty(const std::string& name,
                                                const std::string& value);
#endif // WT_TARGET_JAVA

  /*
   * The DOM root object. This contains not only the application root but
   * also other invisible objects (timers, dialog covers, ...).
   */
  WWebWidget *domRoot() const;

  /*
   * A phony DOM root object, used to logically contain all widgets bound
   * in widgetset mode.
   */
  WContainerWidget *domRoot2() const { return domRoot2_.get(); }

  /*
   * Encode an object to a string, to make it referencable from JavaScript.
   * Currently only used to encode the drag object in drag & drop.
   *
   * FIXME: provide a way to remove the encoding!
   *
   * \see decodeObject()
   */
  std::string encodeObject(WObject *object);

  /*
   * Decode an object.
   *
   * \see encodeObject()
   */
  WObject *decodeObject(const std::string& objectId) const;

#ifndef WT_TARGET_JAVA
  /*! \brief Initializes the application, post-construction.
   *
   * This method is invoked by the %Wt library after construction of a
   * new application. You may reimplement this method to do additional
   * initialization that is not possible from the constructor
   * (e.g. which uses virtual methods).
   */
  virtual void initialize();

  /*! \brief Finalizes the application, pre-destruction.
   *
   * This method is invoked by the %Wt library before destruction of a
   * new application. You may reimplement this method to do additional
   * finalization that is not possible from the destructor (e.g. which
   * uses virtual methods).
   */
  virtual void finalize();
#else
  /*! \brief Destroys the application session.
   *
   * The application is destroyed when the session is invalidated. You
   * should put here any logic which is needed to cleanup the
   * application session.
   *
   * The default implementation does nothing.
   */
  virtual void destroy();
#endif //WT_TARGET_JAVA

  /*! \brief Changes the threshold for two-phase rendering.
   *
   * This changes the threshold for the \p size of a JavaScript
   * response (in bytes) to render invisible changes in one go. If the
   * bandwidth for rendering the invisible changes exceed the
   * threshold, they will be fetched in a second communication, after
   * the visible changes have been rendered.
   *
   * The value is a trade-off: setting it smaller will always use
   * two-phase rendering, increasing the total render time but
   * reducing the latency for the visible changes. Setting it too
   * large will increase the latency to render the visible changes,
   * since first also all invisible changes need to be computed and
   * received in the browser.
   *
   * \if cpp
   * The initial value is read from the configuration file, see \ref
   * config_general.
   * \endif
   */
  void setTwoPhaseRenderingThreshold(int size);

  /*! \brief Sets a new cookie.
   *
   * Use cookies to transfer information across different sessions
   * (e.g. a user name). In a subsequent session you will be able to
   * read this cookie using WEnvironment::getCookie().  You cannot use
   * a cookie to store information in the current session.
   *
   * For more information on how to configure cookies, see the Http::Cookie
   * class.
   *
   * \if cpp
   * \note %Wt provides session tracking automatically, and may be configured
   *       to use a cookie for this. You only need to use cookies yourself
   *       if you want to remember some information (like a logged in identity)
   *       <i>across sessions</i>.
   * \endif
   *
   * \sa WEnvironment::supportsCookies(), WEnvironment::getCookie()
   */
  void setCookie(const Http::Cookie& cookie);

  /*! \brief Sets a new cookie.
   *
   * Use cookies to transfer information across different sessions
   * (e.g. a user name). In a subsequent session you will be able to
   * read this cookie using WEnvironment::getCookie(). You cannot use
   * a cookie to store information in the current session.
   *
   * The name must be a valid cookie name (of type 'token': no special
   * characters or separators, see RFC2616 page 16). The value may be
   * anything. Specify the maximum age (in seconds) after which the
   * client must discard the cookie. To delete a cookie, use a value of '0'.
   *
   * By default the cookie only applies to the application deployment
   * path (WEnvironment::deploymentPath()) in the current domain. To
   * set a proper value for domain, see also RFC2109.
   *
   * \if cpp
   * \note %Wt provides session tracking automatically, and may be configured
   *       to use a cookie for this. You only need to use cookies yourself
   *       if you want to remember some information (like a logged in identity)
   *       <i>across sessions</i>.
   * \endif
   *
   * \sa WEnvironment::supportsCookies(), WEnvironment::getCookie()
   *
   * \deprecated Use setCookie(const Http::Cookie&) instead.
   */
  WT_DEPRECATED("Use setCookie(const Http::Cookie&) instead, the Http::Cookie class allows easier configuration of cookie attributes.")
  void setCookie(const std::string& name, const std::string& value,
                 int maxAge, const std::string& domain = "",
                 const std::string& path = "", bool secure = false);

#ifndef WT_TARGET_JAVA
  void setCookie(const std::string& name, const std::string& value,
                 const WDateTime& expires, const std::string& domain = "",
                 const std::string& path = "", bool secure = false);
#endif // WT_TARGET_JAVA

  /*! \brief Removes a cookie.
   *
   * The cookie will be removed if it has the same name, domain and path as the original
   * cookie (RFC-6265, section 5.3.11).
   *
   * \sa setCookie(const Http::Cookie&)
   */
  void removeCookie(const Http::Cookie& cookie);

  /*! \brief Removes a cookie.
   *
   * \sa setCookie()
   *
   * \deprecated Use removeCookie(const Http::Cookie&) instead.
   */
  WT_DEPRECATED("Use removeCookie(const Http::Cookie&) instead, the Http::Cookie class allows easier configuration of cookie attributes.")
  void removeCookie(const std::string& name, const std::string& domain = "",
		    const std::string& path = "");

  /*! \brief Adds an HTML meta link.
   *
   * When a link was previously set for the same \p href, its contents
   * are replaced.
   * When an empty string is used for the arguments \p media, \p hreflang,
   * \p type or \p sizes, they will be ignored.
   *
   * \sa removeMetaLink()
   */
  void addMetaLink(const std::string &href,
                   const std::string &rel,
                   const std::string &media,
                   const std::string &hreflang,
                   const std::string &type,
                   const std::string &sizes,
                   bool disabled);

  /*! \brief Removes the HTML meta link.
   *
   * \sa addMetaLink()
   */
  void removeMetaLink(const std::string &href);

  /*! \brief Adds a "name" HTML meta header.
   *
   * \sa addMetaHeader(MetaHeaderType, const std::string&, const WString&, const std::string&)
   */
  void addMetaHeader(const std::string& name, const WString& content,
                     const std::string& lang = "");

  /*! \brief Adds an HTML meta header.
   *
   * This method sets either a "name" meta headers, which configures a
   * document property, or a "http-equiv" meta headers, which defines
   * a HTTP headers (but these latter headers are being deprecated).
   *
   * A meta header can however only be added in the following situations:
   *
   * - when a plain HTML session is used (including when the user agent is a
   *   bot), you can add meta headers at any time.
   * - or, when \ref progressive_bootstrap "progressive bootstrap" is
   *   used, you can set meta headers for any type of session, from
   *   within the application constructor (which corresponds to the
   *   initial request).
   * - but never for a Wt::EntryPointType::WidgetSet mode application since then the
   *   application is hosted within a foreign HTML page.
   *
   * These situations coincide with WEnvironment::ajax() returning \c
   * false (see environment()). The reason that it other cases the
   * HTML page has already been rendered, and will not be rerendered
   * since all updates are done dynamically.
   *
   * As an alternative, you can use the &lt;meta-headers&gt;
   * configuration property in the configuration file, which will be
   * applied in all circumstances.
   *
   * \sa removeMetaHeader()
   */
  void addMetaHeader(MetaHeaderType type, const std::string& name,
                     const WString& content, const std::string& lang = "");

  /*! \brief Returns a meta header value.
   *
   * \sa addMetaHeader()
   */
  WString metaHeader(MetaHeaderType type, const std::string& name) const;

  /*! \brief Removes one or all meta headers.
   *
   * Removes the meta header with given type and name (if it is present).
   * If name is empty, all meta headers of the given type are removed.
   *
   * \sa addMetaHeader()
   */
  void removeMetaHeader(MetaHeaderType type, const std::string& name = "");

#ifndef WT_TARGET_JAVA
  /*! \brief Adds an entry to the application log.
   *
   * Starts a new log entry of the given \p type in the %Wt
   * application log file. This method returns a stream-like object to
   * which the message may be streamed.
   *
   * \if cpp
   * A typical usage would be:
   * \code
   *  wApp->log("notice") << "User " << userName << " logged in successfully.";
   * \endcode
   *
   * This would create a log entry that looks like:
   * \verbatim
[2008-Jul-13 14:01:17.817348] 16879 [/app.wt Z2gCmSxIGjLHD73L] [notice] "User bart logged in successfully."
   * \endverbatim
   * \endif
   *
   * \if cpp
   * \sa \ref config_general
   * \endif
   */
  WLogEntry log(const std::string& type) const;
#endif // WT_TARGET_JAVA

  /*! \brief Sets the loading indicator.
   *
   * The loading indicator is shown to indicate that a response from
   * the server is pending or JavaScript is being evaluated.
   *
   * The default loading indicator is a WDefaultLoadingIndicator.
   */
  void setLoadingIndicator(std::unique_ptr<WLoadingIndicator> indicator);

  /*! \brief Returns the loading indicator.
   *
   * \sa setLoadingIndicator()
   */
  WLoadingIndicator *loadingIndicator() const { return loadingIndicator_; }

  /*
   * A url to a resource that provides a one pixel gif. This is sometimes
   * useful for CSS hackery to make IE behave.
   */
  std::string onePixelGifUrl();

  /*
   * The doctype used to deliver the application.
   */
  std::string docType() const;

  /*! \brief Quits the application.
   *
   * This quits the application with a default restart message resolved
   * as WString::tr("Wt.QuittedMessage").
   *
   * \sa quit(const WString&)
   */
  void quit();

  /*! \brief Quits the application.
   *
   * The method returns immediately, but has as effect that the
   * application will be terminated after the current event is
   * completed.
   *
   * The current widget tree (including any modifications still
   * pending and applied during the current event handling) will still
   * be rendered, after which the application is terminated.
   *
   * If the restart message is not empty, then the user will be
   * offered to restart the application (using the provided message)
   * when further interacting with the application.
   *
   * \sa redirect()
   */
  void quit(const WString& restartMessage);

  /*! \brief Returns whether the application has quit.
   *
   * \sa quit()
   */
  bool hasQuit() const { return quitted_; }

  /*! \brief Returns the current maximum size of a request to the
   *         application.
   *
   * The returned value is the maximum request size in bytes.
   *
   * \if cpp
   * The maximum request size is configured in the configuration file,
   * see \ref config_general.
   * \endif
   *
   * \sa requestTooLarge()
   */
  ::int64_t maximumRequestSize() const;

  /*! \brief %Signal which indicates that too a large request was received.
   *
   * The integer parameter is the request size that was received in bytes.
   */
  Signal< ::int64_t>& requestTooLarge() { return requestTooLarge_; }

  /** @name Global keyboard and mouse events
   */
  //!@{
  /*! \brief Event signal emitted when a keyboard key is pushed down.
   *
   * The application receives key events when no widget currently
   * has focus. Otherwise, key events are handled by the widget in focus,
   * and its ancestors.
   *
   * \sa See WInteractWidget::keyWentDown()
   */
  EventSignal<WKeyEvent>& globalKeyWentDown();

  /*! \brief Event signal emitted when a "character" was entered.
   *
   * The application receives key events when no widget currently
   * has focus. Otherwise, key events are handled by the widget in focus,
   * and its ancestors.
   *
   * \sa See WInteractWidget::keyPressed()
   */
  EventSignal<WKeyEvent>& globalKeyPressed();

  /*! \brief Event signal emitted when a keyboard key is released.
   *
   * The application receives key events when no widget currently
   * has focus. Otherwise, key events are handled by the widget in focus,
   * and its ancestors.
   *
   * \sa See WInteractWidget::keyWentUp()
   */
  EventSignal<WKeyEvent>& globalKeyWentUp();

  /*! \brief Event signal emitted when enter was pressed.
   *
   * The application receives key events when no widget currently
   * has focus. Otherwise, key events are handled by the widget in focus,
   * and its ancestors.
   *
   * \sa See WInteractWidget::enterPressed()
   */
  EventSignal<>& globalEnterPressed();

  /*! \brief Event signal emitted when escape was pressed.
   *
   * The application receives key events when no widget currently
   * has focus. Otherwise, key events are handled by the widget in focus,
   * and its ancestors.
   *
   * \sa See WInteractWidget::escapePressed()
   */
  EventSignal<>& globalEscapePressed();
  //!@}

  /*
   * Returns whether debug was configured.
   * (should be public API ?)
   */
  bool debug() const;

  /*
   * Methods for client-side focus
   */
  void setFocus(const std::string& id, int selectionStart, int selectionEnd);

#ifdef WT_DEBUG_JS
  void loadJavaScript(const char *jsFile);
#else
#ifdef WT_TARGET_JAVA
  /*! \brief Loads an internal JavaScript file.
   *
   * This is an internal function and should not be called directly.
   *
   * \sa require(), doJavaScript()
   */
#endif
  void loadJavaScript(const char *jsFile, const WJavaScriptPreamble& preamble);
#endif

  bool javaScriptLoaded(const char *jsFile) const;

  /*! \brief Sets the message for the user to confirm closing of the
   *         application window/tab.
   *
   * If the message is empty, then the user may navigate away from the page
   * without confirmation.
   *
   * Otherwise the user will be prompted with a browser-specific
   * dialog asking him to confirm leaving the page. This \p message is
   * added to the page.
   *
   * \sa unload()
   */
  void setConfirmCloseMessage(const WString& message);

  void enableInternalPaths();

  // should we move this into an InternalPaths utility class / namespace ?
#ifdef WT_TARGET_JAVA
  /*! \brief Utility function to check if one path falls under another path.
   *
   * This returns whether the \p query path matches the given \p path,
   * meaning that it is equal to that path or it specifies a more
   * specific sub path of that path.
   */
#endif // WT_TARGET_JAVA
  static bool pathMatches(const std::string& path, const std::string& query);

#ifndef WT_TARGET_JAVA
  /*! \brief Defers rendering of the current event response.
   *
   * This method defers the rendering of the current event response
   * until resumeRendering() is called. This may be used if you do not
   * want to actively block the current thread while waiting for an
   * event which is needed to complete the current event
   * response. Note that this effectively freezes the user interface,
   * and thus you should only do this if you know that the event you
   * are waiting for will arrive shortly, or there is really nothing more
   * useful for the user to do than wait for the action to complete.
   *
   * A typical use case is in conjunction with the Http::Client, to
   * defer the rendering while waiting for the Http::Client to
   * complete.
   *
   * The function may be called multiple times and the number of deferral
   * requests is counted. The current response is deferred until as
   * many calls to resumeRendering() have been performed.
   *
   * \sa resumeRendering()
   */
  void deferRendering();

  /*! \brief Resumes rendering of a deferred event response.
   *
   * \sa deferRendering()
   */
  void resumeRendering();
#endif

  /*! \brief Encodes an untrusted URL to prevent referer leaks.
   *
   * This encodes an URL so that in case the session ID is present
   * in the current URL, this session ID does not leak to the refenced
   * URL.
   *
   * %Wt will safely handle URLs in the API (in WImage and WAnchor) but
   * you may want to use this function to encode URLs which you use in
   * WTemplate texts.
   */
  std::string encodeUntrustedUrl(const std::string& url) const;

  /*! \brief Pushes a (modal) widget onto the expose stack.
   *
   * This defines a new context of widgets that are currently visible.
   */
  void pushExposedConstraint(WWidget *w);
  void popExposedConstraint(WWidget *w);

  void addGlobalWidget(WWidget *w); // from within constructor
  void removeGlobalWidget(WWidget *w); // from within destructor

  /*! \brief Suspend the application.
   *
   * Keep this application alive for a certain amount of time, while
   * allowing the user to navigate away from the page. This can be
   * useful when using 3rd party login or payment providers.
   * You can later return to the application with a url that includes
   * the session ID as query parameter (see WApplication::url()).
   */
  void suspend(std::chrono::seconds duration);

  /*! \brief Signal that is emitted when the application is no longer suspended.
   *
   * This can be used to apply changes which were difficult to do as a result of
   * the application not being rendered.
   * Eg. Wt uses this to trigger a login as a result of single sign-on.
   */
  Signal<>& unsuspended() { return unsuspended_; }

  /*! \brief Returns the font metrics for server-side rendering
   *
   * In case we require the fallback to render things server-side, this
   * will require the construction of font metrics. The application will
   * construct this object only once, as an optimization.
   *
   * In case the object did not yet exist, a new instance is created.
   */
  ServerSideFontMetrics *serverSideFontMetrics();

protected:
  /*! \brief Notifies an event to the application.
   *
   * This method is called by the event loop for propagating an event
   * to the application. It provides a single point of entry for
   * events to the application, besides the application constructor.
   *
   * You may want to reimplement this method for two reasons:
   *
   * - for having a single point for exception handling: while you may want
   *   to catch recoverable exceptions in a more appropriate place, general
   *   (usually fatal) exceptions may be caught here. You will probably
   *   want to catch the same exceptions in the application constructor
   *   in the same way.
   * - you want to manage resource usage during requests. For example, at
   *   the end of request handling, you want to return a database session
   *   back to the pool. Since %notify() is also used for rendering right after
   *   the application is created, this will also clean up resources after
   *   application construction.
   *
   * In either case, you will need to call the base class
   * implementation of %notify(), as otherwise no events will be
   * delivered to your application.
   *
   * The following shows a generic template for reimplementhing this
   * method for both managing request resources and generic exception
   * handling.
   *
   * \if cpp
   * \code
   * MyApplication::notify(const WEvent& event)
   * {
   *    // Grab resources for during request handling
   *    try {
   *      WApplication::notify(event);
   *    } catch (MyException& exception) {
   *      // handle this exception in a central place
   *    }
   *    // Free resources used during request handling
   * }
   * \endcode
   * \elseif java
   * \code
   * void notify(WEvent event) {
   *     // Grab resources for during request handling
   *     try {
   *       super.notify(event);
   *     }  catch (MyException exception) {
   *       // handle this exception in a central place
   *     }
   *     // Free resources used during request handling
   * }
   * \endcode
   * \endif
   *
   * Note that any uncaught exception throw during event handling
   * terminates the session.
   */
  virtual void notify(const WEvent& e);

  /*! \brief Returns whether a widget is exposed in the interface.
   *
   * The default implementation simply returns \c true, unless a modal
   * dialog is active, in which case it returns \c true only for widgets
   * that are inside the dialog.
   *
   * You may want to reimplement this method if you wish to disallow
   * events from certain widgets even when they are inserted in the
   * widget hierachy.
   */
  virtual bool isExposed(WWidget *w) const;

  /*! \brief Progresses to an Ajax-enabled user interface.
   *
   * This method is called when the progressive bootstrap method is used, and
   * support for AJAX has been detected. The default behavior will propagate
   * the WWidget::enableAjax() method through the widget hierarchy.
   *
   * You may want to reimplement this method if you want to make
   * changes to the user-interface when AJAX is enabled. You should
   * always call the base implementation.
   *
   * \sa WWidget::enableAjax()
   */
  virtual void enableAjax();

  /*! \brief Handles a browser unload event.
   *
   * The browser unloads the application when the user navigates away or
   * when he closes the window or tab.
   *
   * When <tt>reload-is-new-session</tt> is set to \c true, then the
   * default implementation of this method terminates this session by
   * calling quit(), otherwise the session is scheduled to expire within
   * seconds (since it may be a refresh).
   *
   * You may want to reimplement this if you want to keep the
   * application running until it times out.
   *
   * \note There is no guarantee that closing the browser tab sends the unload event. This is
   *       because it is at the web browser's discretion whether it still sends requests for a closed tab.
   *       It's also possible that there was no connection upon closing the tab. Sessions that don't
   *       receive the unload event will eventually time out according to the `session-timeout` set in
   *       `wt_config.xml` (this defaults to 10 minutes).
   */
  virtual void unload();

  /*! \brief Idle timeout handler
   *
   * \if cpp
   * If <tt>idle-timeout</tt> is set in the configuration, this method is called when
   * the user seems idle for the number of seconds set in <tt>idle-timeout</tt>.
   * \elseif java
   * If idle timeout is set in the configuration
   * ({@link Configuration#setIdleTimeout(int)}), this
   * method is called when the user seems idle for the number of seconds set as the
   * idle timeout.
   * \endif
   *
   * This feature can be useful in security sensitive applications
   * to prevent unauthorized users from taking over the session
   * of a user that has moved away from or left behind
   * the device from which they are accessing the %Wt application.
   *
   * The default implementation logs that a timeout has occurred,
   * and calls quit().
   *
   * This method can be overridden to specify different timeout behaviour,
   * e.g. to show a dialog that a user's session has expired, or that
   * the session is about to expire.
   *
   * \if cpp
   *
   * Example for an expiration dialog:
   *
   * \code
   * class MyApplication : public Wt::WApplication {
   * public:
   *   MyApplication(Wt::WEnvironment &env)
   *    : WApplication(env)
   *   { }
   *
   * protected:
   *   virtual void idleTimeout() override
   *   {
   *     if (idleTimeoutDialog_)
   *       return; // Prevent multiple dialogs
   *
   *     idleTimeoutDialog_ = addChild(std::make_unique<Wt::WDialog>("Idle timeout"));
   *     idleTimeoutDialog_->contents()->addNew<Wt::WText>("This session will automatically quit in 1 minute, "
   *                                                       "press 'abort' to continue using the application");
   *     auto btn = idleTimeoutDialog_->footer()->addNew<Wt::WPushButton>("abort");
   *     btn->clicked().connect([this]{
   *       removeChild(idleTimeoutDialog_.get());
   *     });
   *     auto timer = idleTimeoutDialog_->addChild(std::make_unique<Wt::WTimer>());
   *     timer->setInterval(std::chrono::seconds{60});
   *     timer->setSingleShot(true);
   *     timer->timeout().connect([this]{
   *       quit();
   *     });
   *     timer->start();
   *     idleTimeoutDialog_->show();
   *   }
   *
   * private:
   *   Wt::Core::observing_ptr<Wt::WDialog> idleTimeoutDialog_;
   * };
   * \endcode
   *
   * \endif
   *
   * \note The events currently counted as user activity are:
   *  - mousedown
   *  - mouseup
   *  - wheel
   *  - keydown
   *  - keyup
   *  - touchstart
   *  - touchend
   *  - pointerdown
   *  - pointerup
   */
  virtual void idleTimeout();

  /**
   * @brief handleJavaScriptError print javaScript errors to log file.
   * You may want to overwrite it to render error page for example.
   *
   * @param errorText the error will usually be in json format.
   */
  virtual void handleJavaScriptError(const std::string& errorText);
private:
  Signal< ::int64_t > requestTooLarge_;
  Signal<> unsuspended_;

  struct ScriptLibrary {
    ScriptLibrary(const std::string& uri, const std::string& symbol);

    std::string uri, symbol, beforeLoadJS;
    bool operator< (const ScriptLibrary& other) const;
    bool operator== (const ScriptLibrary& other) const;
  };

  struct MetaLink {
    MetaLink(const std::string &href,
             const std::string &rel,
             const std::string &media,
             const std::string &hreflang,
             const std::string &type,
             const std::string &sizes,
             bool disabled);

    std::string href;
    std::string rel;
    std::string media;
    std::string hreflang;
    std::string type;
    std::string sizes;
    bool disabled;
  };

#ifndef WT_TARGET_JAVA
  typedef std::map<std::string, EventSignalBase *> SignalMap;
  typedef std::map<std::string, WResource*> ResourceMap;
#else
  typedef std::weak_value_map<std::string, EventSignalBase *> SignalMap;
  typedef std::weak_value_map<std::string, WResource*> ResourceMap;
#endif
  typedef std::map<std::string, WObject *> ObjectMap;

  /*
   * Basic application stuff
   */
  WebSession *session_; // session owning this application
#ifndef WT_CNOR
  std::weak_ptr<WebSession> weakSession_; // used to sense destruction
#endif // WT_CNOR
  WString title_, closeMessage_;
  bool titleChanged_, closeMessageChanged_, localeChanged_;
  std::unique_ptr<WContainerWidget> domRoot_; // main DOM root
  WContainerWidget *widgetRoot_;  // widgets in main DOM root
  WContainerWidget *timerRoot_;   // timers in main DOM root
  std::unique_ptr<WContainerWidget> domRoot2_; // other virtual root
  WCssStyleSheet styleSheet_;  // internal stylesheet
  std::unique_ptr<WCombinedLocalizedStrings> localizedStrings_;
  WLocale locale_;
  std::string renderedInternalPath_, newInternalPath_;
  Signal<std::string> internalPathChanged_, internalPathInvalid_;
  bool internalPathIsChanged_, internalPathDefaultValid_, internalPathValid_;
  int serverPush_;
  bool serverPushChanged_;
#ifndef WT_TARGET_JAVA
  boost::pool<boost::default_user_allocator_new_delete> *eventSignalPool_;
#endif // WT_TARGET_JAVA
  std::string javaScriptClass_;
  bool quitted_;
  WString quittedMessage_;
  std::unique_ptr<WResource> onePixelGifR_;
  bool internalPathsEnabled_;
  WWidget *exposedOnly_;
  WLoadingIndicator *loadingIndicator_;
  std::string htmlClass_, bodyClass_;
  bool bodyHtmlClassChanged_, enableAjax_;
#ifndef WT_TARGET_JAVA
  bool initialized_;
#endif // WT_TARGET_JAVA
  std::string focusId_;
  int selectionStart_, selectionEnd_;
  LayoutDirection layoutDirection_;
  std::unordered_map<std::string, std::string> htmlAttributes_;
  std::unordered_map<std::string, std::string> bodyAttributes_;
  bool htmlAttributeChanged_, bodyAttributeChanged_;

  std::vector<ScriptLibrary> scriptLibraries_;
  int scriptLibrariesAdded_;

  std::shared_ptr<WTheme> theme_;
  std::vector<WLinkedCssStyleSheet> styleSheets_;
  std::vector<WLinkedCssStyleSheet> styleSheetsToRemove_;

  int styleSheetsAdded_;

  std::vector<MetaHeader> metaHeaders_;
  std::vector<MetaLink> metaLinks_;

  SignalMap exposedSignals_;   // signals that may be accessed
  ResourceMap exposedResources_; // resources that may be accessed
#ifndef WT_TARGET_JAVA
  std::map<WResource*, WWebSocketResource*> exposedWebSocketResources_; // link between exposed resource and their websocket "interface"
#endif // WT_TARGET_JAVA
  ObjectMap encodedObjects_;   // objects encoded for internal purposes
                                 // like 'virtual pointers' (see D&D)
  std::set<std::string> justRemovedSignals_;

  bool exposeSignals_; // if we are currently exposing signals (see WViewWidget)

  std::string afterLoadJavaScript_, beforeLoadJavaScript_;
  int newBeforeLoadJavaScript_;
  std::string autoJavaScript_;
  bool autoJavaScriptChanged_;

#ifndef WT_DEBUG_JS
  std::vector<WJavaScriptPreamble> javaScriptPreamble_;
  int newJavaScriptPreamble_;
#else
  std::vector<const char *> newJavaScriptToLoad_;
#endif // WT_DEBUG_JS
  std::set<const char *> javaScriptLoaded_;
  bool customJQuery_;

  EventSignal<> showLoadingIndicator_, hideLoadingIndicator_;
  JSignal<> unloaded_;
  JSignal<> idleTimeout_;

  // Track cookies added over application lifetime.
  // WEnvironment does not update itself, so `setCookie` is not reflected by it.
  WEnvironment::CookieMap addedCookies_;
  const std::string* findAddedCookies(const std::string& name) const;
  // Remove the added cookie, for correct bookkeeping.
  void removeAddedCookies(const std::string& name);

  WContainerWidget *timerRoot() const { return timerRoot_; }
  WEnvironment& env(); // short-hand for session_->env()

  const std::unordered_map<std::string, std::string>& htmlAttributes() const { return htmlAttributes_; }
  const std::unordered_map<std::string, std::string>& bodyAttributes() const { return bodyAttributes_; }

  /*
   * Functions for exposed signals, resources, and objects
   */
  void addExposedSignal(EventSignalBase* signal);
  void removeExposedSignal(EventSignalBase* signal);
  EventSignalBase  *decodeExposedSignal(const std::string& signalName) const;
  std::string encodeSignal(const std::string& objectId,
                           const std::string& name) const;

  SignalMap& exposedSignals() { return exposedSignals_; }
  std::set<std::string>& justRemovedSignals() { return justRemovedSignals_; }

  std::string resourceMapKey(WResource *resource);
  std::string addExposedResource(WResource *resource);
  bool removeExposedResource(WResource *resource);
  WResource *decodeExposedResource(const std::string& resourceMapKey) const;
  WResource *decodeExposedResource(const std::string& resourceMapKey,
                                   unsigned long rand) const;

#ifndef WT_TARGET_JAVA
  // Manipulation of the link between websocket resources and resources

  // Adds a (private) WWebSocketResource to the application.
  // It functions similarly to simply adding a normal WResource
  void addWebSocketResource(WWebSocketResource* webSocketResource);
  void removeWebSocketResource(WWebSocketResource* webSocketResource);
  WWebSocketResource* findMatchingWebSocketResource(WResource* resource) const;
#endif // WT_TARGET_JAVA

  /*
   * Methods for application state handling
   */
  bool changeInternalPath(const std::string& path);
  bool changedInternalPath(const std::string& path);

  /*
   * Methods for accessing javaScript, which may have erase-on-read
   * semantics
   */
  void streamAfterLoadJavaScript(WStringStream& out);
  void streamBeforeLoadJavaScript(WStringStream& out, bool all);
  void streamJavaScriptPreamble(WStringStream& out, bool all);
#ifdef WT_DEBUG_JS
  void loadJavaScriptFile(WStringStream& out, const char *jsFile);
#endif // WT_DEBUG_JS

  /*
   * Methods that control exposing of signals
   */
  void setExposeSignals(bool how) { exposeSignals_ = how; }
  bool exposeSignals() const { return exposeSignals_; }
  void doUnload();
  void doIdleTimeout();

#ifndef WT_TARGET_JAVA
  int startWaitingAtLock();
  void endWaitingAtLock(int id);
#endif // WT_TARGET_JAVA

  std::string focus() const { return focusId_; }
  int selectionStart() const { return selectionStart_; }
  int selectionEnd() const { return selectionEnd_; }

  WLocalizedStrings *localizedStringsPack();

  /*
   * Methods for audio handling
   */
  SoundManager *getSoundManager();
  SoundManager *soundManager_;

  // Server-side font metrics, constructed once (on demand),
  // and reused by all painters that require it.
  std::unique_ptr<ServerSideFontMetrics> serverSideFontMetrics_;

  static const char *RESOURCES_URL;

#ifdef WT_TARGET_JAVA
  JSlot showLoadJS;
  JSlot hideLoadJS;
#endif

  friend class Auth::AuthModel;
  friend class WCssStyleSheet;
  friend class WebRenderer;
  friend class WebSession;
  friend class WebController;
  friend class EventSignalBase;
  friend class JavaScriptEvent;
  friend class UpdateLockImpl;
  friend class WContainerWidget;
  friend class WDialog;
  friend class WFileUpload;
  friend class WInteractWidget;
  friend class WLineEdit;
  friend class WMenu;
  friend class WResource;
  friend class WSound;
  friend class WString;
  friend class WTextArea;
  friend class WTimer;
  friend class WViewWidget;
  friend class WWidget;
#ifndef WT_TARGET_JAVA
  friend class WWebSocketResource;
#endif // WT_TARGET_JAVA
  friend class WWebWidget;
};

#ifndef WT_TARGET_JAVA
#ifdef DOXYGEN_ONLY
/*! \brief Runs the %Wt application server.
 *
 * This function runs the application server, and should be called
 * only once (e.g. from within your main function).
 *
 * The \p createApplication parameter is a <tt>std::function</tt>
 * object that should create a new application instance for a new user
 * visiting the application. It is of type:
 * <tt>std::function<Wt::WApplication* (const
 * Wt::WEnvironment&)></tt>, and thus you can pass to it a function
 * like:
 *
 * <pre>
 * Wt::WApplication *createApplication(const Wt::WEnvironment& env)
 * {
 *   // ...
 * }
 * </pre>
 *
 * When using the built-in httpd, the implementation listens for POSIX
 * termination signals (or console CTRL-C) event. You can use the
 * WServer class for more flexible control on starting and stopping
 * the server.
 *
 * \relates WServer
 * \sa WApplication
 */
extern int WRun(int argc, char** argv,
                ApplicationCreator createApplication = 0);
#else // DOXYGEN_ONLY
extern int WTCONNECTOR_API
WRun(int argc, char** argv,
     ApplicationCreator createApplication = ApplicationCreator());

#endif // DOXYGEN_ONLY

#ifdef DOXYGEN_ONLY
/*! \brief Runs the %Wt application server.
 *
 * This function runs the application server, and should be called
 * only once (e.g. from within your main function).
 *
 * The \p createApplication parameter is a <tt>std::function</tt>
 * object that should create a new application instance for a new user
 * visiting the application. It is of type:
 * <tt>std::function<Wt::WApplication* (const
 * Wt::WEnvironment&)></tt>, and thus you can pass to it a function
 * like:
 *
 * <pre>
 * Wt::WApplication *createApplication(const Wt::WEnvironment& env)
 * {
 *   // ...
 * }
 * </pre>
 *
 * When using the built-in httpd, the implementation listens for POSIX
 * termination signals (or console CTRL-C) event. You can use the
 * WServer class for more flexible control on starting and stopping
 * the server.
 *
 * This version of WRun() takes a std::string
 * for the application path, and a vector of arguments (not including
 * argv[0], the application path) instead of argc and argv,
 * for better convenience when arguments are not provided via
 * the command line.
 *
 * \relates WServer
 * \sa WApplication
 */
extern int WRun(const std::string &applicationPath,
                const std::vector<std::string> &args,
                ApplicationCreator createApplication = 0);
#else // DOXYGEN_ONLY
extern int WTCONNECTOR_API
WRun(const std::string &applicationPath,
     const std::vector<std::string> &args,
     ApplicationCreator createApplication = ApplicationCreator());

#endif // DOXYGEN_ONLY
#endif // WT_TARGET_JAVA

/*! \def wApp
 *  \brief Global constant for accessing the application instance.
 *
 * This is equivalent to WApplication::instance()
 *
 * \relates WApplication
 */
#define wApp Wt::WApplication::instance()

}

#endif // WAPPLICATION_
