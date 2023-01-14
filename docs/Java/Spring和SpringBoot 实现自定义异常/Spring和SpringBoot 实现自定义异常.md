# Spring 和 Spring Boot 实现自定义异常

---

本文将演示**如何使用 Spring 和 Spring Boot 中实现 REST API 的异常处理**，并了解不同版本引入了哪些新功能。  
**在 Spring 3.2 之前，在 Spring MVC 中处理异常的两种主要方法是：HandlerExceptionResolver 或 @ExceptionHandler 注解。**  这两种方法都有一些明显的缺点。  
**从 3.2 开始，我们就使用 @ControllerAdvice 注解来解决前两种方案的局限性，** 并在整个应用程序中促进统一的异常处理。  
现在，**Spring 5 引入了 ResponseStatusException 类：**  REST API 中快速处理基本异常的方法。  
最后，我们将看到 Spring Boot 带来了什么，以及如何配置它以满足我们的需要。

第一个解决方案在 Controller 级别工作——我们将定义一个处理异常的方法，并使用 @ExceptionHandler 注释它：

    public class FooController{
         
        
        @ExceptionHandler({ CustomException1.class, CustomException2.class })
        public void handleException() {
            
        }
    }

这种方法有一个主要的缺点—— **@ExceptionHandler 注释方法只对特定的 Controller，而不是对整个应用程序全局。** 当然，将它添加到每个控制器并不适合一般的异常处理机制。  
我们可以通过**让所有的 Controller 扩展一个 Base Controller 来绕过这个限制** —— 但是，对于应用程序来说，无论出于什么原因，这都可能是一个问题。例如，控制器可能已经从另一个基类扩展而来，这个基类可能在另一个 jar 中，或者不能直接修改，或者它们本身不能直接修改。  
接下来，我们将研究解决异常处理问题的另一种方法—一种全局的方法。

第二个解决方案是定义一个 HandlerExceptionResolver —— **它将解决应用程序抛出的任何异常。** 它还允许我们在 REST API 中实现统一的异常处理机制。  
在使用自定义解析器之前，让我们回顾一下现有的实现。

## ExceptionHandlerExceptionResolver

该解析器是在 Spring 3.1 中引入的，默认情况下在 DispatcherServlet 中启用。 这实际上是前面介绍的 @ExceptionHandler 机制如何工作的核心组件。

## DefaultHandlerExceptionResolver

这个解析器是在 Spring 3.0 中引入的，在 DispatcherServlet 中默认启用它。它用于解决相应 HTTP 状态码的标准 Spring 异常，即客户端异常 - 4xx 和服务器异常 - 5xx 状态码。下面是它处理的 Spring 异常的完整列表：

| Exception                               | HTTP Status Code             |
| --------------------------------------- | ---------------------------- |
| BindException                           | 400 (Bad Request)            |
| ConversionNotSupportedException         | 500 (Internal Server Error)  |
| HttpMediaTypeNotAcceptableException     | 406 (Not Acceptable)         |
| HttpMediaTypeNotSupportedException      | 415 (Unsupported Media Type) |
| HttpMessageNotReadableException         | 400 (Bad Request)            |
| HttpMessageNotWritableException         | 500 (Internal Server Error)  |
| HttpRequestMethodNotSupportedException  | 405 (Method Not Allowed)     |
| MethodArgumentNotValidException         | 400 (Bad Request)            |
| MissingServletRequestParameterException | 400 (Bad Request)            |
| MissingServletRequestPartException      | 400 (Bad Request)            |
| NoSuchRequestHandlingMethodException    | 404 (Not Found)              |
| TypeMismatchException                   | 400 (Bad Request)            |

虽然它确实正确地设置了响应的状态代码，但有一个限制是**它没有为响应的主体设置任何内容。** 对于 REST API —— 状态代码实际上并没有足够的信息提供给客户端 —— 响应也必须有一个主体，以允许应用程序提供关于故障的附加信息。  
这可以通过配置视图解析和通过 ModelAndView 呈现异常内容来解决，但是这个解决方案显然不是最优的。这就是为什么 Spring 3.2 引入了一个更好的选项，我们将在下面讨论。

## ResponseStatusExceptionResolver

这个解析器也是在 Spring 3.0 中引入的，并在 DispatcherServlet 中默认启用。它的主要职责是使用自定义异常上可用的 @ResponseStatus 注解，并将这些异常映射到 HTTP 状态码。  
这样的自定义异常如下所示：

    @ResponseStatus(value = HttpStatus.NOT_FOUND)
    public class MyResourceNotFoundException extends RuntimeException {
        public MyResourceNotFoundException() {
            super();
        }
        public MyResourceNotFoundException(String message, Throwable cause) {
            super(message, cause);
        }
        public MyResourceNotFoundException(String message) {
            super(message);
        }
        public MyResourceNotFoundException(Throwable cause) {
            super(cause);
        }
    }

与 DefaultHandlerExceptionResolver 一样，这个解析器在处理响应主体的方式上是有限的——它确实将状态代码映射到响应上，但是响应的主体仍然是空的。

## SimpleMappingExceptionResolver 和 AnnotationMethodHandlerExceptionResolver

SimpleMappingExceptionResolver 已经出现了很长一段时间了——它来自于旧的 Spring MVC 模型，**并且与 REST 服务无关。** 我们基本上使用它来映射异常类名来查看名称。  
在 Spring 3.0 中引入了 AnnotationMethodHandlerExceptionResolver 来通过 @ExceptionHandler 注释处理异常，但是从 Spring 3.2 开始， ExceptionHandlerExceptionResolver 就不再支持它了。

## Custom HandlerExceptionResolver

DefaultHandlerExceptionResolver 和 ResponseStatusExceptionResolver 的组合在为 Spring RESTful 服务提供良好的异常处理机制方面走了很长的路。缺点是 —— 如前所述一样无法控制响应的主体。  
这种方法是 Spring REST 服务异常处理的一致且易于配置的机制。但是，它确实有局限性：它与低级别的 HtttpServletResponse 交互，并且它适合使用 ModelAndView 的旧 MVC 模型 —— 所以仍然有改进的空间。

Spring 3.2 通过 @ControllerAdvice 注释为全局 @ExceptionHandler 带来了支持。这使得一种机制能够脱离旧的 MVC 模型，利用 ResponseEntity 以及 @ExceptionHandler 的类型安全和灵活性：

    @ControllerAdvice
    public class RestResponseEntityExceptionHandler 
      extends ResponseEntityExceptionHandler {
     
        @ExceptionHandler(value 
          = { IllegalArgumentException.class, IllegalStateException.class })
        protected ResponseEntity<Object> handleConflict(
          RuntimeException ex, WebRequest request) {
            String bodyOfResponse = "This should be application specific";
            return handleExceptionInternal(ex, bodyOfResponse, 
              new HttpHeaders(), HttpStatus.CONFLICT, request);
        }
    }

@ControllerAdvice 注解允许我们**将之前分散的多个 @ExceptionHandlers 合并为一个全局异常处理组件。**  
实际的机制非常简单，而且非常灵活。它给我们带来了：

-   完全控制响应的主体以及状态码
-   将多个异常映射到同一方法，并一起处理
-   它很好地利用了更新的 RESTful ResposeEntity 响应

Spring 5 引入了 ResponseStatusException 类。 我们可以创建一个提供 HttpStatus 以及可能的原因：

    @GetMapping(value = "/{id}")
    public Foo findById(@PathVariable("id") Long id, HttpServletResponse response) {
        try {
            Foo resourceById = RestPreconditions.checkFound(service.findOne(id));
     
            eventPublisher.publishEvent(new SingleResourceRetrievedEvent(this, response));
            return resourceById;
         }
        catch (MyResourceNotFoundException exc) {
             throw new ResponseStatusException(
               HttpStatus.NOT_FOUND, "Foo Not Found", exc);
        }
    }

**使用 ResponseStatusException 有什么好处?**

-   我们可以快速实现基本的解决方案
-   一种类型，多种状态代码：一种异常类型可能导致多种不同的响应。**与 @ExceptionHandler 相比，这减少了耦合性**
-   不需要创建那么多的自定义异常类
-   由于可以通过编程方式创建异常，因此可以**更好地控制异常处理**

**那么如何权衡呢?**

-   没有统一的异常处理方法：与提供全局方法的 @ControllerAdvice 相比，执行一些应用程序范围的约定更加困难
-   代码复制：我们可能会发现自己在多个 Controller 中复制代码

我们还应该注意到，可以在一个应用程序中组合不同的方法。  
例如，**我们可以全局实现 @ControllerAdvice，但也可以在本地实现 responsestatusexception。** 然而，我们需要小心：如果同一个异常可以用多种方式处理，我们可能会注意到一些令人惊讶的行为。一种可能的约定始终以一种方式处理着一种特定类型的异常。

当通过身份验证的用户尝试访问他没有足够权限访问的资源时，将发生拒绝访问。

## MVC –自定义异常页面

首先，让我们看一下该解决方案的 MVC 风格，看看如何为 Access Denied 自定义异常页面：

### XML 配置：

    <http>
        <intercept-url pattern="/admin/*" access="hasAnyRole('ROLE_ADMIN')"/>   
        ... 
        <access-denied-handler error-page="/my-error-page" />
    </http>

### Java 配置：

    @Override
    protected void configure(HttpSecurity http) throws Exception {
        http.authorizeRequests()
            .antMatchers("/admin/*").hasAnyRole("ROLE_ADMIN")
            ...
            .and()
            .exceptionHandling().accessDeniedPage("/my-error-page");
    }

当用户试图在没有足够权限的情况下访问资源时，他们将被重定向到 “/my-error-page”。

## 自定义 AccessDeniedHandler

接下来，让我们看看如何编写我们的自定义 AccessDeniedHandler：

    @Component
    public class CustomAccessDeniedHandler implements AccessDeniedHandler {
     
        @Override
        public void handle
          (HttpServletRequest request, HttpServletResponse response, AccessDeniedException ex) 
          throws IOException, ServletException {
            response.sendRedirect("/my-error-page");
        }
    }

现在，我们使用 XML 对其进行配置：

    <http>
        <intercept-url pattern="/admin/*" access="hasAnyRole('ROLE_ADMIN')"/> 
        ...
        <access-denied-handler ref="customAccessDeniedHandler" />
    </http>

或使用 Java 进行配置：

    @Autowired
    private CustomAccessDeniedHandler accessDeniedHandler;
     
    @Override
    protected void configure(HttpSecurity http) throws Exception {
        http.authorizeRequests()
            .antMatchers("/admin/*").hasAnyRole("ROLE_ADMIN")
            ...
            .and()
            .exceptionHandling().accessDeniedHandler(accessDeniedHandler)
    }

注意，在我们的 CustomAccessDeniedHandler 中，我们可以按照自己的意愿通过重定向或显示自定义异常消息来定制响应。

**Spring Boot 提供了一个 ErrorController 实现，以合理的方式处理异常。**  
简而言之，它为浏览器提供一个后备异常页面（又称 Whitelabel 异常页面），并为 RESTful 非 HTML 请求提供了一个 JSON 响应：

    {
        "timestamp": "2019-01-17T16:12:45.977+0000",
        "status": 500,
        "error": "Internal Server Error",
        "message": "Error processing the request!",
        "path": "/my-endpoint-with-exceptions"
    }

和往常一样，Spring Boot 允许使用 properties 配置以下功能：

-   server.error.whitelabel.enabled：可用于禁用 Whitelabel 异常页面并依靠 servlet 容器提供 HTML 异常消息
-   server.error.include-stacktrace：使用 always，可以在 HTML 和 JSON 默认响应中都包含了 stacktrace

除了这些属性之外，**我们还可以为 /error 提供自己的视图，覆盖 Whitelabel 页面。**  
我们还可以通过在上下文中包含 ErrorAttributes bean 来定制希望在响应中显示的属性。我们可以扩展由 Spring Boot 提供的 DefaultErrorAttributes 类，使事情变得更简单：

    @Component
    public class MyCustomErrorAttributes extends DefaultErrorAttributes {
     
        @Override
        public Map<String, Object> getErrorAttributes(
          WebRequest webRequest, boolean includeStackTrace) {
            Map<String, Object> errorAttributes = 
              super.getErrorAttributes(webRequest, includeStackTrace);
            errorAttributes.put("locale", webRequest.getLocale()
                .toString());
            errorAttributes.remove("error");
     
            
     
            return errorAttributes;
        }
    }

如果我们想进一步定义（或覆盖）应用程序如何处理特定内容类型的异常，则可以注册一个 ErrorController bean。  
同样，我们可以利用 Spring Boot 提供的默认 BasicErrorController 来帮助我们。  
例如，假设我们希望自定义应用程序如何处理 XML 中触发的异常。我们所要做的就是使用 @RequestMapping 定义一个公共方法，并声明它产生了 application/xml 类型：

    @Component
    public class MyErrorController extends BasicErrorController {
     
        public MyErrorController(ErrorAttributes errorAttributes) {
            super(errorAttributes, new ErrorProperties());
        }
     
        @RequestMapping(produces = MediaType.APPLICATION_XML_VALUE)
        public ResponseEntity<Map<String, Object>> xmlError(HttpServletRequest request) {
             
        
     
        }
    }

## 返回给前端错误信息

```yml
server:
  address: 0.0.0.0
  port: 8899
  error:
    # 使错误信息带有message
    include-message: always
    whitelabel:
      enabled: true
```
