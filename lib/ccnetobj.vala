

namespace Ccnet {

public const string EVENT_USER_ROLE = "user-role";
public const string EVENT_USER_MYROLE = "user-myrole";
public const string EVENT_GROUP_FOLLOW = "group-follow";
public const string EVENT_GROUP_NOT_FOLLOW = "group-not-follow";

/*
public class Event : Object {

    public string _etype;
    public string etype { 
        get { return _etype; }
        set { _etype = value; }
    }

    public int _ctime;
    public int ctime { 
        get { return _ctime; }
        set { _ctime = value; }
    }

    public string _body;
    public string body {
        get { return _body; }
        set { _body = value; }
    }
}
*/
public class Proc : Object {

    public string peer_name { get; set; }

    public string _name;
    public string name { 
        get { return _name; }
        set { _name = value; }
    }

    public int _ctime;
    public int ctime { 
        get { return _ctime; }
        set { _ctime = value; }
    }

    public int _dtime;   //dead time
    public int dtime {
        get { return _dtime; }
        set { _dtime = value; }
    }

}

public class EmailUser : Object {

    public int id { get; set; }
    public string email { get; set; }
    public string passwd { get; set; }
    public bool is_staff { get; set; }
    public bool is_active { get; set; }
    public int64 ctime { get; set; }
}

public class Group : Object {

    public int id { get; set; }
    public string group_name { get; set; }
    public string creator_name { get; set; }
    public int64 timestamp { get; set; }
}

public class GroupUser : Object {

    public int group_id { get; set; }
    public string user_name { get; set; }
    public int is_staff { get; set; }
}

} // namespace
