# HTTP包装返回值

## HttpResponse.java

```java
package ihep.daq.online.response;

import org.springframework.http.HttpStatus;

import io.swagger.annotations.ApiModel;
import io.swagger.annotations.ApiModelProperty;
import lombok.Getter;
import lombok.Setter;

@Getter
@Setter
@ApiModel(description = "Http api 返回")
public class HttpResponse {
    @ApiModelProperty(value = "错误码")
    private int status;
    @ApiModelProperty(value = "消息")
    private String message;
    @ApiModelProperty(value = "错误")
    private String error;
    @ApiModelProperty(value = "数据")
    private Object data;

    public HttpResponse(int status, String msg, String err, Object data) {
        this.status = status;
        this.message = msg;
        this.error = err;
        this.data = data;
    }

    /**
     * @param data 要返回的数据
     * @return HttpResponse
     */
    static public HttpResponse success(Object data) {
        return new HttpResponse(HttpStatus.OK.value(), HttpStatus.OK.name(), "", data);
    }

    /**
     * @param status org.springframework.http.HttpStatus
     * @return HttpResponse
     */
    static public HttpResponse fail(HttpStatus status) {
        return new HttpResponse(status.value(), "", status.name(), null);
    }

    /**
     * @param status org.springframework.http.HttpStatus
     * @param msg 错误信息
     * @return HttpResponse
     */
    static public HttpResponse fail(HttpStatus status, String msg) {
        return new HttpResponse(status.value(), msg, status.name(), null);
    }
}
```

## HttpResponseData

```java
package ihep.daq.online.response;

import io.swagger.annotations.ApiModel;
import io.swagger.annotations.ApiModelProperty;
import lombok.Getter;
import lombok.Setter;

@Getter
@Setter
@ApiModel(value = "Http api 返回data字段")
public class HttpResponseData<T> {
    @ApiModelProperty(value = "分页页码")
    private int page;
    @ApiModelProperty(value = "下一页的页码")
    private int nextPage;
    @ApiModelProperty(value = "本页元素数量")
    private int itemNumber;
    @ApiModelProperty(value = "返回元素集合")
    private T items;
}
```
