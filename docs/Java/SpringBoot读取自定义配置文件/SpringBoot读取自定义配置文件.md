# SpringBoot读取自定义配置文件

[toc]

---
## @ConfigurationProperties（或@Value）和@PropertySource

```java
@ConfigurationProperties(prefix = "spring.data.mongodb")
@PropertySource(value = {"custom.yaml"}, factory = YamlPropertySourceFactory.class ,ignoreResourceNotFound = false)
public class MongoCustomProperties {
    private String dbName;
    private String collection;
}
```

## YamlPropertySourceFactory
spring boot不能识别自定义yaml，需要自己实现YamlPropertySourceFactory

```java
import java.io.IOException;
import java.util.Properties;

import org.springframework.beans.factory.config.YamlPropertiesFactoryBean;
import org.springframework.core.env.PropertiesPropertySource;
import org.springframework.core.env.PropertySource;
import org.springframework.core.io.support.EncodedResource;
import org.springframework.core.io.support.PropertySourceFactory;

public class YamlPropertySourceFactory implements PropertySourceFactory {

    @Override
    public PropertySource<?> createPropertySource(String name, EncodedResource resource) throws IOException {
        YamlPropertiesFactoryBean factory = new YamlPropertiesFactoryBean();
        factory.setResources(resource.getResource());
        factory.afterPropertiesSet();
        Properties ymlProperties = factory.getObject();
        String propertyName = name != null ? name : resource.getResource().getFilename();
        return new PropertiesPropertySource(propertyName, ymlProperties);
    }
}
```